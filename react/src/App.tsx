import { useEffect, useMemo, useState } from 'react'
import GoalViewer from './section/goal-view'
import {
  createMockGoalListFromTemplate,
  findGoalByGlobalIndex,
  type Goal,
} from './goal'

type RouteName = 'home' | 'goal'

function getLocationState() {
  const route: RouteName = window.location.pathname === '/goal' ? 'goal' : 'home'
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
  const mockGoals = useMemo(() => createMockGoalListFromTemplate(), [])
  const [route, setRoute] = useState<RouteName>(initialLocation.route)
  const [goalIndex, setGoalIndex] = useState<number | null>(initialLocation.goalIndex)
  const [message, setMessage] = useState(
    `Using mock goals only. Loaded ${mockGoals.length} items locally.`,
  )

  const selectedGoal = useMemo<Goal | null>(() => {
    const fallbackIndex = mockGoals[0]?.globalIndex ?? null
    const targetIndex = goalIndex ?? fallbackIndex

    if (!targetIndex) {
      return null
    }

    return findGoalByGlobalIndex(mockGoals, targetIndex) ?? mockGoals[0] ?? null
  }, [goalIndex, mockGoals])

  useEffect(() => {
    const onPopState = () => {
      const locationState = getLocationState()
      setRoute(locationState.route)
      setGoalIndex(locationState.goalIndex)
    }

    window.addEventListener('popstate', onPopState)
    return () => window.removeEventListener('popstate', onPopState)
  }, [])

  useEffect(() => {
    if (route !== 'goal') {
      return
    }

    if (selectedGoal) {
      setGoalIndex((current) => current ?? selectedGoal.globalIndex)
      return
    }

    setMessage('No mock goal available.')
    window.history.replaceState({}, '', '/')
    setRoute('home')
    setGoalIndex(null)
  }, [route, selectedGoal])

  function viewFirstGoal() {
    if (!selectedGoal) {
      setMessage('No mock goals are available.')
      return
    }

    setMessage(`Viewing mock goal: ${selectedGoal.title}`)
    setGoalIndex(selectedGoal.globalIndex)
    window.history.pushState({}, '', `/goal?goal=${selectedGoal.globalIndex}`)
    setRoute('goal')
  }

  if (route === 'goal') {
    if (!selectedGoal) {
      return <p className="goal-connect-message">No mock goal available.</p>
    }

    return <GoalViewer goal={selectedGoal} />
  }

  return (
    <>
      <button
        type="button"
        className="goal-view-button"
        onClick={viewFirstGoal}
      >
        View mock goal
      </button>
      <p className="goal-connect-message">{message}</p>
    </>
  )
}

export default App
