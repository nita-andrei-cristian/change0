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

  /**
   * Payload expected by POST /goal/decompose.
   * The server accepts goalIndex, globalIndex, goal-index, goalId, id.
   * We keep the canonical form used in the JS bootstrap.
   */
  toDecomposePayload(): GoalDecomposePayload {
    return { goalIndex: this.globalIndex }
  }

  /**
   * Returns the canonical decomposition endpoint URL.
   */
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

/**
 * Finds a goal by its globalIndex, matching the server's identifier scheme.
 */
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

  if (hours === 0 && minutes === 0) {
    return `${remainingSeconds}s`
  }

  if (hours === 0) {
    return `${minutes}m ${remainingSeconds}s`
  }

  if (minutes === 0 && remainingSeconds === 0) {
    return `${hours}h`
  }

  if (remainingSeconds === 0) {
    return `${hours}h ${minutes}m`
  }

  return `${hours}h ${minutes}m ${remainingSeconds}s`
}

export function formatGoalDate(value: number | null) {
  if (!value) {
    return 'not set'
  }

  const date = new Date(value * 1000)
  if (Number.isNaN(date.getTime())) {
    return 'invalid date'
  }

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
