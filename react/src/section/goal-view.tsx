import { useEffect, useRef } from 'react'
import type { Goal } from '../goal';

function resizeCanvas(canvas: HTMLCanvasElement) {
  const context = canvas.getContext('2d')
  if (!context) {
    return null
  }

  const rect = canvas.getBoundingClientRect()
  const scale = Math.max(1, Math.min(window.devicePixelRatio || 1, 2))
  const width = Math.max(1, Math.round(rect.width))
  const height = Math.max(1, Math.round(rect.height))

  canvas.width = Math.round(width * scale)
  canvas.height = Math.round(height * scale)
  context.setTransform(scale, 0, 0, scale, 0, 0)

  return { context, width, height }
}

function drawCanvas(canvas: HTMLCanvasElement, goal : Goal) {
  const frame = resizeCanvas(canvas)
  if (!frame) {
    return
  }

  const { context, width, height } = frame
  context.fillStyle = '#000000'
  context.fillRect(0, 0, width, height)

  const cx = width/2, cy = height/2;

  context.fillStyle = "#ffffff";
  const fontSize = 32;
  context.font = `Roboto ${fontSize}px`;

  const text_m_info = context.measureText(goal.title);
  
  context.fillText(goal.title, cx - text_m_info.width/2, cy - fontSize / 2);
}

export interface GoalViewerProps {
  goal: Goal
}

export default function GoalViewer({ goal }: GoalViewerProps) {
  const canvasRef = useRef<HTMLCanvasElement | null>(null)

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas) {
      return
    }

    const redraw = () => drawCanvas(canvas, goal)
    redraw()

    window.addEventListener('resize', redraw)
    return () => window.removeEventListener('resize', redraw)
  }, [])

  return (
    <canvas
      ref={canvasRef}
      className="goal-viewer-canvas"
      aria-label={goal?.title ? `Goal viewer canvas for ${goal.title}` : 'Goal viewer canvas'}
    />
  )
}

