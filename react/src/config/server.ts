export const DEFAULT_SERVER_HOST = '127.0.0.1'
export const DEFAULT_SERVER_PORT = 8085
export const SERVER_BASE_URL = `http://${DEFAULT_SERVER_HOST}:${DEFAULT_SERVER_PORT}`

export const SERVER_ENDPOINTS = {
  goalCreate: `${SERVER_BASE_URL}/goal/create`,
  goalEvents: `${SERVER_BASE_URL}/goal/events`,
  goalList: `${SERVER_BASE_URL}/goal/list`,
  goalDecompose: `${SERVER_BASE_URL}/goal/decompose`,
} as const

export function buildGoalDecomposeUrl(baseUrl = SERVER_BASE_URL) {
  return `${baseUrl}/goal/decompose`
}

export function buildGoalListUrl(baseUrl = SERVER_BASE_URL) {
  return `${baseUrl}/goal/list`
}
