import { useEffect, useRef, useState } from 'react';

function formatTimestamp(timestamp: number): string {
  return new Date(timestamp).toLocaleTimeString([], {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  });
}

/**
 * State of a test runner screen: its runner state, the current step shown in
 * the layout and a timestamped live log. Log and step updates are dropped once
 * the screen unmounts, since a run can outlive it.
 */
export function useRunLog<State extends string>(
  initialState: State,
  initialStep: string
) {
  const isMountedRef = useRef(true);
  const [runnerState, setRunnerState] = useState<State>(initialState);
  const [currentStep, setCurrentStep] = useState(initialStep);
  const [liveLog, setLiveLog] = useState<string[]>([]);

  useEffect(() => {
    isMountedRef.current = true;
    return () => {
      isMountedRef.current = false;
    };
  }, []);

  const appendLog = (message: string) => {
    if (isMountedRef.current) {
      setLiveLog((previous) => [
        ...previous,
        `${formatTimestamp(Date.now())} ${message}`,
      ]);
    }
  };

  const showStep = (message: string) => {
    if (isMountedRef.current) {
      setCurrentStep(message);
    }
  };

  const clearLog = () => setLiveLog([]);

  return {
    isMountedRef,
    runnerState,
    setRunnerState,
    currentStep,
    showStep,
    liveLog,
    appendLog,
    clearLog,
  };
}
