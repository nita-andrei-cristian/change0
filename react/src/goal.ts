import {
  SERVER_BASE_URL,
  buildGoalDecomposeUrl,
  buildGoalListUrl,
} from './config/server'

export type GoalRelation = 'parent' | 'next' | 'prev'

export interface GoalInit {
  id: string
  title: string
  extraInfo: string
  startDate: number | null
  endDate: number | null
  requiredTime: number
  subgoals: number[]
  parent: number | null
  prev: number | null
  next: number | null
  globalIndex: number
  depth: number
  retryDepth: number
  priority: number
}

export interface GoalDecomposePayload {
  goalIndex: number
}

export interface GoalListResponseItem {
  id: string
  globalIndex: number
  title: string
  extra_info: string
  start_date: number
  end_date: number
  required_time: number
  subgoals: number[]
  parent: number
  prev: number
  next: number
  depth: number
  retry_depth: number
  priority: number
}

export interface GoalListResponse {
  ok: boolean
  count: number
  container_len: number
  goals: GoalListResponseItem[]
}

export class Goal implements GoalInit {
  static readonly fields: ReadonlyArray<keyof GoalInit> = [
    'id',
    'title',
    'extraInfo',
    'startDate',
    'endDate',
    'requiredTime',
    'subgoals',
    'parent',
    'prev',
    'next',
    'globalIndex',
    'depth',
    'retryDepth',
    'priority',
  ]

  id: string
  title: string
  extraInfo: string
  startDate: number | null
  endDate: number | null
  requiredTime: number
  subgoals: number[]
  parent: number | null
  prev: number | null
  next: number | null
  globalIndex: number
  depth: number
  retryDepth: number
  priority: number

  constructor(init: GoalInit) {
    this.id = init.id
    this.title = init.title
    this.extraInfo = init.extraInfo
    this.startDate = init.startDate
    this.endDate = init.endDate
    this.requiredTime = init.requiredTime
    this.subgoals = [...init.subgoals]
    this.parent = init.parent
    this.prev = init.prev
    this.next = init.next
    this.globalIndex = init.globalIndex
    this.depth = init.depth
    this.retryDepth = init.retryDepth
    this.priority = init.priority
  }

  static from(init: GoalInit | Goal) {
    return init instanceof Goal ? init : new Goal(init)
  }

  static fromServer(item: GoalListResponseItem) {
    return new Goal({
      id: item.id,
      title: item.title,
      extraInfo: item.extra_info,
      startDate: item.start_date > 0 ? item.start_date : null,
      endDate: item.end_date > 0 ? item.end_date : null,
      requiredTime: item.required_time,
      subgoals: [...item.subgoals],
      parent: item.parent > 0 ? item.parent : null,
      prev: item.prev > 0 ? item.prev : null,
      next: item.next > 0 ? item.next : null,
      globalIndex: item.globalIndex,
      depth: item.depth,
      retryDepth: item.retry_depth,
      priority: item.priority,
    })
  }

  toDecomposePayload(): GoalDecomposePayload {
    return { goalIndex: this.globalIndex }
  }

  static decomposeUrl(baseUrl = SERVER_BASE_URL) {
    return buildGoalDecomposeUrl(baseUrl)
  }

  formatRequiredTime() {
    return formatGoalDuration(this.requiredTime)
  }

  formatStartDate() {
    return formatGoalDate(this.startDate)
  }

  formatEndDate() {
    return formatGoalDate(this.endDate)
  }
}

export interface GoalEdge {
  from: number
  to: number
  relation: GoalRelation
}

export function findGoalByGlobalIndex(goals: Goal[], globalIndex: number) {
  return goals.find((goal) => goal.globalIndex === globalIndex) ?? null
}

export function getGoalChildren(goals: Goal[], goalIndex: number) {
  return goals.filter((goal) => goal.parent === goalIndex)
}

export function formatGoalDuration(seconds: number) {
  const total = Math.max(0, Math.trunc(seconds))
  const hours = Math.floor(total / 3600)
  const minutes = Math.floor((total % 3600) / 60)
  const remainingSeconds = total % 60

  if (hours === 0 && minutes === 0) return `${remainingSeconds}s`
  if (hours === 0) return `${minutes}m ${remainingSeconds}s`
  if (minutes === 0 && remainingSeconds === 0) return `${hours}h`
  if (remainingSeconds === 0) return `${hours}h ${minutes}m`

  return `${hours}h ${minutes}m ${remainingSeconds}s`
}

export function formatGoalDate(value: number | null) {
  if (!value) return 'not set'

  const date = new Date(value * 1000)
  if (Number.isNaN(date.getTime())) return 'invalid date'

  return date.toLocaleString()
}

export async function loadGoalsFromServer(baseUrl = SERVER_BASE_URL) {
  const response = await fetch(buildGoalListUrl(baseUrl), {
    method: 'GET',
    cache: 'no-store',
  })

  if (!response.ok) {
    throw new Error(`Failed to load goals: ${response.status}`)
  }

  const payload = (await response.json()) as GoalListResponse
  const items = Array.isArray(payload.goals) ? payload.goals : []

  return items.map(Goal.fromServer)
}

/* -------------------------------------------------------------------------- */
/* Mock goals                                                                  */
/* -------------------------------------------------------------------------- */

export interface MockGoalTemplate {
  title: string
  extraInfo?: string
  requiredTime: number
  priority?: number
  startDate?: number | null
  endDate?: number | null
  children?: MockGoalTemplate[]
}

export function createMockGoalListFromTemplate(
  rootTitle = 'Build a production-quality MMO RPG',
): Goal[] {
  const hour = 3600
  const minute = 60
  const now = Math.floor(Date.now() / 1000)

  const template: MockGoalTemplate = {
    title: rootTitle,
    extraInfo: 'Root production goal. Total time is elapsed/total time, not focused work time.',
    requiredTime: 50 * hour,
    priority: 100,
    startDate: now - 2 * hour,
    endDate: null,
    children: [
      {
        title: 'Define the vertical slice scope and constraints',
        extraInfo: 'Current scheduled branch. Decompose this first before moving right.',
        requiredTime: 12 * hour,
        priority: 100,
        startDate: now - 2 * hour,
        endDate: null,
        children: [
          {
            title: 'Pin down the minimum viable MMO slice',
            extraInfo: 'Choose the smallest complete playable loop.',
            requiredTime: 3 * hour,
            priority: 100,
            startDate: now - 2 * hour,
            endDate: null,
            children: [
              {
                title: 'Write the core player loop',
                extraInfo: 'Combat, reward, progression, return-to-hub loop.',
                requiredTime: 55 * minute,
                priority: 100,
                startDate: now - 90 * minute,
                endDate: now - 35 * minute,
              },
              {
                title: 'List must-have multiplayer behaviors',
                extraInfo: 'Party visibility, shared enemies, state sync, persistence.',
                requiredTime: 45 * minute,
                priority: 95,
                startDate: now - 30 * minute,
                endDate: null,
              },
            ],
          },
          {
            title: 'Specify class, role, and party assumptions',
            extraInfo: 'Define combat roles before architecture work.',
            requiredTime: 2 * hour,
            priority: 90,
            startDate: null,
            endDate: null,
            children: [
              {
                title: 'Draft the initial class matrix',
                extraInfo: 'Tank, damage, support, healer assumptions.',
                requiredTime: 50 * minute,
                priority: 90,
              },
              {
                title: 'Define party size and matchmaking rules',
                extraInfo: 'Minimum assumptions for multiplayer encounters.',
                requiredTime: 40 * minute,
                priority: 85,
              },
            ],
          },
          {
            title: 'Define the progression cadence',
            extraInfo: 'Decide how fast players unlock power.',
            requiredTime: 2 * hour,
            priority: 80,
            children: [
              {
                title: 'Sketch level and loot pacing',
                extraInfo: 'Early-game reward cadence.',
                requiredTime: 45 * minute,
                priority: 80,
              },
            ],
          },
          {
            title: 'Write the explicit out-of-scope bounds',
            extraInfo: 'Prevent scope creep.',
            requiredTime: 90 * minute,
            priority: 70,
            children: [
              {
                title: 'Create the not-now list',
                extraInfo: 'Raids, guild economy, crafting depth, large world streaming.',
                requiredTime: 35 * minute,
                priority: 70,
              },
            ],
          },
          {
            title: 'Validate the slice definition against time budget',
            extraInfo: 'Check whether the current slice fits the 50h total-time envelope.',
            requiredTime: 90 * minute,
            priority: 65,
            children: [
              {
                title: 'Review decomposition and trim scope',
                extraInfo: 'Remove or defer anything exceeding budget.',
                requiredTime: 30 * minute,
                priority: 65,
              },
            ],
          },
        ],
      },
      {
        title: 'Design the multiplayer architecture',
        extraInfo: 'Generated after the current branch has enough confidence.',
        requiredTime: 10 * hour,
        priority: 80,
        children: [
          {
            title: 'Choose authoritative server model',
            extraInfo: 'Decide what state is owned by server versus client.',
            requiredTime: 55 * minute,
            priority: 80,
          },
          {
            title: 'Define world/session topology',
            extraInfo: 'Instances, shards, zones, rooms, or hybrid.',
            requiredTime: 50 * minute,
            priority: 75,
          },
        ],
      },
      {
        title: 'Implement player combat, movement, and interaction',
        extraInfo: 'Core gameplay implementation branch.',
        requiredTime: 12 * hour,
        priority: 75,
        children: [
          {
            title: 'Implement movement sync prototype',
            extraInfo: 'Client prediction or simple authoritative sync.',
            requiredTime: 60 * minute,
            priority: 75,
          },
          {
            title: 'Implement basic combat exchange',
            extraInfo: 'Targeting, damage, cooldowns, server validation.',
            requiredTime: 55 * minute,
            priority: 72,
          },
        ],
      },
      {
        title: 'Add progression, loot, and persistence',
        extraInfo: 'State that survives sessions.',
        requiredTime: 9 * hour,
        priority: 65,
        children: [],
      },
      {
        title: 'Package and test the playable vertical slice',
        extraInfo: 'Final validation branch.',
        requiredTime: 7 * hour,
        priority: 55,
        children: [],
      },
    ],
  }

  return createMockGoals(template)
}

export function createMockGoals(template: MockGoalTemplate): Goal[] {
  const goals: Goal[] = []
  let nextGlobalIndex = 1

  function visit(
    node: MockGoalTemplate,
    depth: number,
    parent: number | null,
    siblingPosition: number,
  ): number {
    const globalIndex = nextGlobalIndex++
    const childIndexes: number[] = []

    const goal = new Goal({
      id: `mock-goal-${globalIndex}`,
      title: node.title,
      extraInfo: node.extraInfo ?? '',
      startDate: node.startDate ?? null,
      endDate: node.endDate ?? null,
      requiredTime: node.requiredTime,
      subgoals: childIndexes,
      parent,
      prev: null,
      next: null,
      globalIndex,
      depth,
      retryDepth: 0,
      priority: node.priority ?? Math.max(1, 100 - depth * 10 - siblingPosition),
    })

    goals.push(goal)

    const children = node.children ?? []

    for (let i = 0; i < children.length; i += 1) {
      const childIndex = visit(children[i], depth + 1, globalIndex, i)
      childIndexes.push(childIndex)
    }

    for (let i = 0; i < childIndexes.length; i += 1) {
      const child = findGoalByGlobalIndex(goals, childIndexes[i])
      if (!child) continue

      child.prev = i > 0 ? childIndexes[i - 1] : null
      child.next = i < childIndexes.length - 1 ? childIndexes[i + 1] : null
    }

    goal.subgoals = childIndexes

    return globalIndex
  }

  visit(template, 0, null, 0)

  return goals
}

/* -------------------------------------------------------------------------- */
/* Goal map preparation for GLSL                                                */
/* -------------------------------------------------------------------------- */

export type InferredGoalState = 'idle' | 'started' | 'finished'

export interface PreparedGoalNode {
  id: string
  parentId: number | null
  depthAbsolute: number
  depthRelative: number
  x: number
  y: number
  radius: number
  height: number
  stateValue: number
  progress: number
  parentIndex: number
  groupId: number
  globalIndex: number
}

export interface InteractionNode {
  id: string
  title: string
  x: number
  y: number
  radius: number
  state: InferredGoalState
  progress: number
  parentId: number | null
  parentIndex: number
  globalIndex: number
}

export interface PrepareGoalMapOptions {
  maxDepthRelative?: number
  rootRadius?: number
  containmentFactor?: number
  siblingPadding?: number
  minRadius?: number
  maxVisibleGoals?: number
  heightBase?: number
  heightDepthFalloff?: number
  now?: number
}

export interface PreparedGoalMapData {
  visibleGoals: PreparedGoalNode[]
  shaderData: Float32Array
  textureWidth: number
  textureHeight: number
  interactionNodes: InteractionNode[]
}

export function prepareGoalMapData(
  goals: Goal[],
  viewRootIdOrGlobalIndex: string | number,
  options: PrepareGoalMapOptions = {},
): PreparedGoalMapData {
  const opts = {
    maxDepthRelative: options.maxDepthRelative ?? 2,
    rootRadius: options.rootRadius ?? 1,
    containmentFactor: options.containmentFactor ?? 0.88,
    siblingPadding: options.siblingPadding ?? 0.08,
    minRadius: options.minRadius ?? 0.035,
    maxVisibleGoals: options.maxVisibleGoals ?? 256,
    heightBase: options.heightBase ?? 1,
    heightDepthFalloff: options.heightDepthFalloff ?? 0.72,
    now: options.now ?? Math.floor(Date.now() / 1000),
  }

  const byGlobalIndex = new Map<number, Goal>()
  const byId = new Map<string, Goal>()

  for (const goal of goals) {
    byGlobalIndex.set(goal.globalIndex, goal)
    byId.set(goal.id, goal)
  }

  const viewRoot =
    typeof viewRootIdOrGlobalIndex === 'number'
      ? byGlobalIndex.get(viewRootIdOrGlobalIndex)
      : byId.get(viewRootIdOrGlobalIndex)

  if (!viewRoot) {
    throw new Error(`prepareGoalMapData: view root not found: ${viewRootIdOrGlobalIndex}`)
  }

  const visible = collectVisibleGoalObjects(
    goals,
    viewRoot.globalIndex,
    opts.maxDepthRelative,
    opts.maxVisibleGoals,
  )

  const visibleSet = new Set(visible.map((goal) => goal.globalIndex))
  const prepared: PreparedGoalNode[] = []

  const rootNode: PreparedGoalNode = {
    id: viewRoot.id,
    parentId: viewRoot.parent,
    depthAbsolute: viewRoot.depth,
    depthRelative: 0,
    x: 0,
    y: 0,
    radius: opts.rootRadius,
    height: opts.heightBase,
    stateValue: goalStateToValue(inferGoalState(viewRoot, opts.now)),
    progress: inferGoalProgress(viewRoot, opts.now),
    parentIndex: -1,
    groupId: 0,
    globalIndex: viewRoot.globalIndex,
  }

  prepared.push(rootNode)

  layoutGoalChildrenRecursive({
    goals,
    parentGoal: viewRoot,
    parentPrepared: rootNode,
    prepared,
    visibleSet,
    opts,
  })

  const shaderData = buildGoalShaderData(prepared)

  const interactionNodes = prepared.map((node) => {
    const source = byGlobalIndex.get(node.globalIndex)
    const state = source ? inferGoalState(source, opts.now) : 'idle'

    return {
      id: node.id,
      title: source?.title ?? '',
      x: node.x,
      y: node.y,
      radius: node.radius,
      state,
      progress: node.progress,
      parentId: node.parentId,
      parentIndex: node.parentIndex,
      globalIndex: node.globalIndex,
    }
  })

  return {
    visibleGoals: prepared,
    shaderData,
    textureWidth: prepared.length,
    textureHeight: 2,
    interactionNodes,
  }
}

function collectVisibleGoalObjects(
  goals: Goal[],
  viewRootGlobalIndex: number,
  maxDepthRelative: number,
  maxVisibleGoals: number,
): Goal[] {
  const byParent = buildGoalsByParent(goals)
  const root = findGoalByGlobalIndex(goals, viewRootGlobalIndex)

  if (!root) return []

  const result: Goal[] = []

  function visit(goal: Goal, depthRelative: number) {
    if (result.length >= maxVisibleGoals) return
    if (depthRelative > maxDepthRelative) return

    result.push(goal)

    const children = byParent.get(goal.globalIndex) ?? []
    const orderedChildren = orderGoalsLeftToRight(children)

    for (const child of orderedChildren) {
      visit(child, depthRelative + 1)
    }
  }

  visit(root, 0)

  return result
}

function layoutGoalChildrenRecursive(args: {
  goals: Goal[]
  parentGoal: Goal
  parentPrepared: PreparedGoalNode
  prepared: PreparedGoalNode[]
  visibleSet: Set<number>
  opts: Required<PrepareGoalMapOptions>
}) {
  const { goals, parentGoal, parentPrepared, prepared, visibleSet, opts } = args

  const children = orderGoalsLeftToRight(getGoalChildren(goals, parentGoal.globalIndex)).filter(
    (child) => visibleSet.has(child.globalIndex),
  )

  if (children.length === 0) return

  const childLayouts = packChildrenInsideParent({
    children,
    parentX: parentPrepared.x,
    parentY: parentPrepared.y,
    parentRadius: parentPrepared.radius,
    containmentFactor: opts.containmentFactor,
    siblingPadding: opts.siblingPadding,
    minRadius: opts.minRadius,
  })

  const parentPreparedIndex = prepared.indexOf(parentPrepared)
  const firstChildIndex = prepared.length

  for (let i = 0; i < children.length; i += 1) {
    const child = children[i]
    const layout = childLayouts[i]
    const depthRelative = parentPrepared.depthRelative + 1
    const state = inferGoalState(child, opts.now)

    prepared.push({
      id: child.id,
      parentId: child.parent,
      depthAbsolute: child.depth,
      depthRelative,
      x: layout.x,
      y: layout.y,
      radius: layout.radius,
      height:
        opts.heightBase *
        Math.pow(opts.heightDepthFalloff, depthRelative) *
        goalStateHeightMultiplier(state),
      stateValue: goalStateToValue(state),
      progress: inferGoalProgress(child, opts.now),
      parentIndex: parentPreparedIndex,
      groupId: firstChildIndex,
      globalIndex: child.globalIndex,
    })
  }

  for (let i = 0; i < children.length; i += 1) {
    layoutGoalChildrenRecursive({
      goals,
      parentGoal: children[i],
      parentPrepared: prepared[firstChildIndex + i],
      prepared,
      visibleSet,
      opts,
    })
  }
}

function packChildrenInsideParent(args: {
  children: Goal[]
  parentX: number
  parentY: number
  parentRadius: number
  containmentFactor: number
  siblingPadding: number
  minRadius: number
}) {
  const {
    children,
    parentX,
    parentY,
    parentRadius,
    containmentFactor,
    siblingPadding,
    minRadius,
  } = args

  const count = children.length
  const usableRadius = parentRadius * containmentFactor

  if (count === 0) return []

  const totalWeight = children.reduce((sum, child) => sum + goalWeight(child), 0)
  const baseArea = Math.PI * usableRadius * usableRadius
  const fillFactor = count <= 3 ? 0.36 : 0.44

  const layouts = children.map((child, i) => {
    const weight = goalWeight(child) / totalWeight
    const radius = Math.max(
      minRadius,
      Math.sqrt((baseArea * fillFactor * weight) / Math.PI),
    )

    const angle = deterministicAngle(child.id, child.title) + i * goldenAngle()
    const ring = count === 1 ? 0 : Math.sqrt((i + 0.5) / count)
    const distance = Math.max(0, usableRadius - radius) * ring * 0.82

    return {
      x: parentX + Math.cos(angle) * distance,
      y: parentY + Math.sin(angle) * distance,
      radius: Math.min(radius, usableRadius * 0.48),
    }
  })

  relaxSiblingOverlap(layouts, {
    parentX,
    parentY,
    usableRadius,
    padding: siblingPadding,
    iterations: 42,
  })

  for (const layout of layouts) {
    constrainCircleInsideParent(layout, parentX, parentY, usableRadius)
  }

  return layouts
}

function relaxSiblingOverlap(
  nodes: Array<{ x: number; y: number; radius: number }>,
  options: {
    parentX: number
    parentY: number
    usableRadius: number
    padding: number
    iterations: number
  },
) {
  const { parentX, parentY, usableRadius, padding, iterations } = options

  for (let iter = 0; iter < iterations; iter += 1) {
    for (let i = 0; i < nodes.length; i += 1) {
      for (let j = i + 1; j < nodes.length; j += 1) {
        const a = nodes[i]
        const b = nodes[j]

        let dx = b.x - a.x
        let dy = b.y - a.y
        let dist = Math.sqrt(dx * dx + dy * dy)

        if (dist < 0.00001) {
          const angle = deterministicAngle(String(i), String(j))
          dx = Math.cos(angle) * 0.001
          dy = Math.sin(angle) * 0.001
          dist = 0.001
        }

        const minDist = a.radius + b.radius + padding * Math.min(a.radius, b.radius)

        if (dist < minDist) {
          const push = (minDist - dist) * 0.5
          const nx = dx / dist
          const ny = dy / dist

          a.x -= nx * push
          a.y -= ny * push
          b.x += nx * push
          b.y += ny * push
        }
      }
    }

    for (const node of nodes) {
      constrainCircleInsideParent(node, parentX, parentY, usableRadius)
    }
  }
}

function constrainCircleInsideParent(
  node: { x: number; y: number; radius: number },
  parentX: number,
  parentY: number,
  usableRadius: number,
) {
  const dx = node.x - parentX
  const dy = node.y - parentY
  const dist = Math.sqrt(dx * dx + dy * dy)
  const maxDist = Math.max(0, usableRadius - node.radius)

  if (dist > maxDist && dist > 0.00001) {
    const scale = maxDist / dist
    node.x = parentX + dx * scale
    node.y = parentY + dy * scale
  }
}

/**
 * Texture layout:
 *
 * Row 0:
 * R = x
 * G = y
 * B = radius
 * A = height
 *
 * Row 1:
 * R = stateValue
 * G = progress
 * B = depthRelative
 * A = parentIndex
 */
export function buildGoalShaderData(nodes: PreparedGoalNode[]) {
  const textureWidth = nodes.length
  const textureHeight = 2
  const data = new Float32Array(textureWidth * textureHeight * 4)

  for (let i = 0; i < nodes.length; i += 1) {
    const node = nodes[i]

    const row0 = i * 4
    data[row0 + 0] = node.x
    data[row0 + 1] = node.y
    data[row0 + 2] = node.radius
    data[row0 + 3] = node.height

    const row1 = textureWidth * 4 + i * 4
    data[row1 + 0] = node.stateValue
    data[row1 + 1] = node.progress
    data[row1 + 2] = node.depthRelative
    data[row1 + 3] = node.parentIndex
  }

  return data
}

function buildGoalsByParent(goals: Goal[]) {
  const byParent = new Map<number, Goal[]>()

  for (const goal of goals) {
    if (goal.parent == null) continue

    const existing = byParent.get(goal.parent) ?? []
    existing.push(goal)
    byParent.set(goal.parent, existing)
  }

  return byParent
}

function orderGoalsLeftToRight(goals: Goal[]) {
  return [...goals].sort((a, b) => {
    if (a.prev === b.globalIndex) return 1
    if (a.next === b.globalIndex) return -1
    if (b.prev === a.globalIndex) return -1
    if (b.next === a.globalIndex) return 1
    return b.priority - a.priority || a.globalIndex - b.globalIndex
  })
}

export function inferGoalState(goal: Goal, _now = Math.floor(Date.now() / 1000)): InferredGoalState {
  if (goal.startDate && goal.endDate) return 'finished'
  if (goal.startDate && !goal.endDate) return 'started'
  return 'idle'
}

export function inferGoalProgress(goal: Goal, now = Math.floor(Date.now() / 1000)) {
  if (goal.startDate && goal.endDate) return 1
  if (!goal.startDate) return 0

  const elapsed = Math.max(0, now - goal.startDate)
  const estimated = Math.max(1, goal.requiredTime)

  return clamp01(elapsed / estimated)
}

function goalStateToValue(state: InferredGoalState) {
  switch (state) {
    case 'idle':
      return 0
    case 'started':
      return 1
    case 'finished':
      return 2
  }
}

function goalStateHeightMultiplier(state: InferredGoalState) {
  switch (state) {
    case 'started':
      return 1.18
    case 'finished':
      return 0.82
    case 'idle':
    default:
      return 1
  }
}

function goalWeight(goal: Goal) {
  const time = Math.max(1, goal.requiredTime)
  const childBonus = 1 + Math.sqrt(goal.subgoals.length) * 0.35
  const priorityBonus = 1 + clamp01(goal.priority / 100) * 0.35

  return Math.sqrt(time) * childBonus * priorityBonus
}

function clamp01(value: number) {
  return Math.min(1, Math.max(0, Number.isFinite(value) ? value : 0))
}

function goldenAngle() {
  return Math.PI * (3 - Math.sqrt(5))
}

function deterministicAngle(id: string, title: string) {
  return hashToUnit(`${id}:${title}`) * Math.PI * 2
}

function hashToUnit(input: string) {
  let hash = 2166136261

  for (let i = 0; i < input.length; i += 1) {
    hash ^= input.charCodeAt(i)
    hash = Math.imul(hash, 16777619)
  }

  return (hash >>> 0) / 4294967295
}
