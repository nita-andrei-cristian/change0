import { useEffect, useState } from 'react'
import GoalViewer from './section/goal-view'
import { findGoalByGlobalIndex, loadGoalsFromServer, type Goal } from './goal'

type RouteName = 'home' | 'goal'

function getLocationState() {
  const route = window.location.pathname === '/goal' ? 'goal' : 'home'
  if (route !== 'goal') {
    return { route, goalIndex: null as number | null }
  }

  const query = new URLSearchParams(window.location.search)
  const goalIndex = Number(query.get('goal'))
  return {
    route,
    goalIndex: Number.isFinite(goalIndex) && goalIndex > 0 ? goalIndex : null,
  }
}

function App() {
  const initialLocation = getLocationState()
  const [route, setRoute] = useState<RouteName>(initialLocation.route)
  const [goalIndex, setGoalIndex] = useState<number | null>(
    initialLocation.goalIndex,
  )
  const [goals, setGoals] = useState<Goal[]>([])
  const [selectedGoal, setSelectedGoal] = useState<Goal | null>(null)
  const [message, setMessage] = useState('Server not connected.')
  const [busy, setBusy] = useState(false)

  useEffect(() => {
    const onPopState = () => {
      const locationState = getLocationState()
      setRoute(locationState.route)
      setGoalIndex(locationState.goalIndex)
      if (locationState.route === 'home') {
        setSelectedGoal(null)
        setGoalIndex(null)
      }
    }

    window.addEventListener('popstate', onPopState)
    return () => window.removeEventListener('popstate', onPopState)
  }, [])

  useEffect(() => {
    if (route !== 'goal' || selectedGoal) {
      return
    }

    let alive = true

    ;(async () => {
      try {
        const loadedGoals = goals.length > 0 ? goals : await loadServerGoals()
        const targetIndex = goalIndex ?? loadedGoals[0]?.globalIndex ?? null
        const goal = targetIndex
          ? findGoalByGlobalIndex(loadedGoals, targetIndex)
          : null

        if (!alive) {
          return
        }

        if (goal) {
          setSelectedGoal(goal)
          setGoals(loadedGoals)
          return
        }

        setMessage('Success. No goals were returned by the server.')
        window.history.replaceState({}, '', '/')
        setRoute('home')
        setGoalIndex(null)
      } catch (error) {
        if (!alive) {
          return
        }

        setMessage(
          error instanceof Error ? error.message : 'Failed to restore goal view.',
        )
        window.history.replaceState({}, '', '/')
        setRoute('home')
        setGoalIndex(null)
      }
    })()

    return () => {
      alive = false
    }
  }, [route, goalIndex, goals, selectedGoal])

  async function loadServerGoals() {
    const loadedGoals = await loadGoalsFromServer()
    setGoals(loadedGoals)
    return loadedGoals
  }

  async function connectServer() {
    setBusy(true)
    setMessage('Connecting to server...')

    try {
      const loadedGoals = await loadServerGoals()
      setMessage(`Success. Loaded ${loadedGoals.length} goals from server.`)
    } catch (error) {
      setMessage(
        error instanceof Error ? error.message : 'Failed to connect to server.',
      )
    } finally {
      setBusy(false)
    }
  }

  async function viewFirstGoal() {
    setBusy(true)
    setMessage('Opening first goal...')

    try {
      const loadedGoals = goals.length > 0 ? goals : await loadServerGoals()
      const firstGoal = findGoalByGlobalIndex(
        loadedGoals,
        loadedGoals[0]?.globalIndex ?? 0,
      )

      if (!firstGoal) {
        setMessage('Success. No goals were returned by the server.')
        return
      }

      setSelectedGoal(firstGoal)
      setGoalIndex(firstGoal.globalIndex)
      window.history.pushState({}, '', `/goal?goal=${firstGoal.globalIndex}`)
      setRoute('goal')
    } catch (error) {
      setMessage(
        error instanceof Error ? error.message : 'Failed to open goal view.',
      )
    } finally {
      setBusy(false)
    }
  }

  if (route === 'goal') {
    if (!selectedGoal) {
      return <p className="goal-connect-message">Loading goal...</p>
    }

    return <GoalViewer goal={selectedGoal} />
  }

  return (
    <>
      <button
        type="button"
        className="goal-connect-button"
        onClick={connectServer}
        disabled={busy}
      >
        Connect server
      </button>
      <button
        type="button"
        className="goal-view-button"
        onClick={viewFirstGoal}
        disabled={busy}
      >
        View goal
      </button>
      <p className="goal-connect-message">{message}</p>
    </>
  )
}

export default App
