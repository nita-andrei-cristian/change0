import { OrbitControls } from '@react-three/drei'
import { Canvas, useFrame, useThree } from '@react-three/fiber'
import { useEffect, useMemo, useRef } from 'react'
import * as THREE from 'three'
import {
  Goal,
  createMockGoalListFromTemplate,
  prepareGoalMapData,
  type PreparedGoalMapData,
} from '../goal'

const vertexShader = `
varying float vHeight;
varying float vStateBlend;
varying float vProgressBlend;

uniform sampler2D uGoalData;
uniform int uGoalCount;
uniform float uGoalTextureWidth;
uniform float uTime;

vec4 readGoalRow(float index, float row) {
  float u = (index + 0.5) / uGoalTextureWidth;
  float v = (row + 0.5) / 2.0;
  return texture2D(uGoalData, vec2(u, v));
}

void main() {
  float accumulatedHeight = 0.0;
  float stateBlend = 0.0;
  float progressBlend = 0.0;
  float totalInfluence = 0.0;
  vec3 transformed = position;

  const int MAX_GOALS = 256;

  for (int i = 0; i < MAX_GOALS; i++) {
    if (i >= uGoalCount) {
      break;
    }

    vec4 row0 = readGoalRow(float(i), 0.0);
    vec4 row1 = readGoalRow(float(i), 1.0);

    vec2 goalPosition = row0.rg;
    float radius = max(row0.b, 0.0001);
    float goalHeight = row0.a;
    float stateValue = row1.r;
    float progress = row1.g;

    float distanceToGoal = distance(position.xy, goalPosition);
    float influence = smoothstep(radius, 0.0, distanceToGoal);
    influence = influence * influence * (3.0 - 2.0 * influence);

    accumulatedHeight += influence * goalHeight;
    stateBlend += influence * stateValue;
    progressBlend += influence * progress;
    totalInfluence += influence;
  }

  if (totalInfluence > 0.0) {
    stateBlend /= totalInfluence;
    progressBlend /= totalInfluence;
  }

  transformed.z += accumulatedHeight * 0.55;

  vHeight = accumulatedHeight;
  vStateBlend = stateBlend;
  vProgressBlend = progressBlend;

  gl_Position = projectionMatrix * modelViewMatrix * vec4(transformed, 1.0);
}
`

const fragmentShader = `
varying float vHeight;
varying float vStateBlend;
varying float vProgressBlend;

void main() {
  vec3 low = vec3(0.055, 0.065, 0.075);
  vec3 mid = vec3(0.16, 0.19, 0.20);
  vec3 high = vec3(0.42, 0.45, 0.40);

  float t = smoothstep(0.0, 1.2, vHeight);
  vec3 color = mix(low, mid, t);
  color = mix(color, high, smoothstep(0.55, 1.4, vHeight));

  float startedAmount = smoothstep(0.7, 1.3, vStateBlend);
  float finishedAmount = smoothstep(1.7, 2.1, vStateBlend);

  color = mix(color, color + vec3(0.02, 0.06, 0.07), startedAmount * 0.65);

  float luminance = dot(color, vec3(0.299, 0.587, 0.114));
  vec3 desaturated = mix(color, vec3(luminance), 0.45);
  color = mix(color, desaturated + vec3(0.015, 0.02, 0.018), finishedAmount * 0.85);

  float contour = 1.0 - smoothstep(0.015, 0.035, abs(fract(vHeight * 10.0) - 0.5));
  color += contour * 0.035;
  color += vProgressBlend * 0.05;

  gl_FragColor = vec4(color, 1.0);
}
`

interface GoalTerrainMeshProps {
  prepared: PreparedGoalMapData
}

function CameraLookAt() {
  const { camera } = useThree()

  useEffect(() => {
    camera.lookAt(0, 0, 0)
    camera.updateProjectionMatrix()
  }, [camera])

  return null
}

function GoalTerrainMesh({ prepared }: GoalTerrainMeshProps) {
  const materialRef = useRef<THREE.ShaderMaterial | null>(null)

  const goalTexture = useMemo(() => {
    const texture = new THREE.DataTexture(
      prepared.shaderData,
      prepared.textureWidth,
      prepared.textureHeight,
      THREE.RGBAFormat,
      THREE.FloatType,
    )

    texture.minFilter = THREE.NearestFilter
    texture.magFilter = THREE.NearestFilter
    texture.wrapS = THREE.ClampToEdgeWrapping
    texture.wrapT = THREE.ClampToEdgeWrapping
    texture.generateMipmaps = false
    texture.needsUpdate = true

    return texture
  }, [prepared.shaderData, prepared.textureHeight, prepared.textureWidth])

  const uniforms = useMemo(
    () => ({
      uGoalData: { value: goalTexture },
      uGoalCount: { value: prepared.visibleGoals.length },
      uGoalTextureWidth: { value: prepared.textureWidth },
      uTime: { value: 0 },
    }),
    [goalTexture, prepared.textureWidth, prepared.visibleGoals.length],
  )

  useEffect(() => {
    return () => {
      goalTexture.dispose()
    }
  }, [goalTexture])

  useFrame((state) => {
    if (materialRef.current) {
      materialRef.current.uniforms.uTime.value = state.clock.elapsedTime
    }
  })

  return (
    <mesh>
      <planeGeometry args={[4, 4, 192, 192]} />
      <shaderMaterial
        ref={materialRef}
        uniforms={uniforms}
        vertexShader={vertexShader}
        fragmentShader={fragmentShader}
        transparent={false}
        side={THREE.DoubleSide}
      />
    </mesh>
  )
}

export interface GoalViewerProps {
  goal: Goal
}

export default function GoalViewer({ goal }: GoalViewerProps) {
  void goal

  const prepared = useMemo(() => {
    const mockGoals = createMockGoalListFromTemplate()
    const viewRootId = mockGoals[0]?.globalIndex

    if (!viewRootId) {
      throw new Error('GoalViewer: mock goals did not produce a root node.')
    }

    return prepareGoalMapData(mockGoals, viewRootId, { maxDepthRelative: 2 })
  }, [])

  return (
    <div
      className="goal-viewer-canvas"
      aria-label="Goal viewer terrain"
      style={{ width: '100%', height: '100%', background: '#050608' }}
    >
      <Canvas
        camera={{ position: [0, -3.2, 2.2], fov: 45 }}
        style={{ width: '100%', height: '100%' }}
        dpr={[1, 2]}
        onCreated={({ gl }) => {
          gl.setClearColor('#050608')
        }}
      >
        <CameraLookAt />
        <GoalTerrainMesh prepared={prepared} />
        <OrbitControls
          enableDamping
          enablePan
          minDistance={1.5}
          maxDistance={7}
          maxPolarAngle={Math.PI * 0.48}
        />
      </Canvas>
    </div>
  )
}
