import { useEffect, useState } from 'react'
import GoalViewer from './section/goal-view'
import { findGoalByGlobalIndex, loadGoalsFromServer, type Goal } from './goal'

type RouteName = 'home' | 'goal'

function getRouteFromLocation() {
  return window.location.pathname === '/goal' ? 'goal' : 'home'
}

function App() {
  const [route, setRoute] = useState<RouteName>(getRouteFromLocation)
  const [goals, setGoals] = useState<Goal[]>([])
  const [selectedGoal, setSelectedGoal] = useState<Goal | null>(null)
  const [message, setMessage] = useState('Server not connected.')
  const [busy, setBusy] = useState(false)

  useEffect(() => {
    const onPopState = () => setRoute(getRouteFromLocation())
    window.addEventListener('popstate', onPopState)
    return () => window.removeEventListener('popstate', onPopState)
  }, [])

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
      window.history.pushState({}, '', '/goal')
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

