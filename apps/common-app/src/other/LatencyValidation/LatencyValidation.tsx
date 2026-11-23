import React, { FC, RefObject, useEffect, useRef, useState } from 'react';
import { Platform } from 'react-native';
import { AudioManager, AudioRecorder } from 'react-native-audio-api';

import { Container } from '../../components';
import * as TestUI from '../../testComponents';
import {
  closeLoopbackAudioIO,
  createLoopbackAudioIO,
  runLoopbackLatencyTest,
  type LoopbackAudioIO,
} from './latencyTests';
import LoopbackAnalysisPanel from './LoopbackAnalysisPanel';
import type { LoopbackAnalysis } from './types';

const READY_MESSAGE = 'Ready to run the speaker-to-mic loopback latency check.';

const verdictMessages: Record<LoopbackAnalysis['scenario']['status'], string> =
  {
    pass: 'Analysis complete: aligned waveform matches the reported latencies.',
    skipped:
      'Analysis complete: microphone capture did not contain a matchable beep pattern.',
    fail: 'Analysis complete: waveform alignment exists but latency delta is outside tolerance.',
  };

/**
 * Releases the running test's audio I/O. Both the run and an unmount call it,
 * and only the first one closes it.
 */
async function releaseRunIO(ioRef: RefObject<LoopbackAudioIO | null>) {
  const io = ioRef.current;
  ioRef.current = null;
  if (io) {
    await closeLoopbackAudioIO(io);
  }
}

const LatencyValidation: FC = () => {
  const runIORef = useRef<LoopbackAudioIO | null>(null);
  const {
    isMountedRef,
    runnerState,
    setRunnerState,
    currentStep,
    showStep,
    liveLog,
    appendLog,
    clearLog,
  } = TestUI.useRunLog<'idle' | 'running'>('idle', READY_MESSAGE);
  const [analysis, setAnalysis] = useState<LoopbackAnalysis | null>(null);

  useEffect(
    () => () => {
      releaseRunIO(runIORef);
    },
    []
  );

  const runTest = async () => {
    if (runnerState === 'running') {
      return;
    }

    // The run reconfigures and finally deactivates the shared audio session,
    // which would end another screen's recording.
    if (AudioRecorder.isRecordingOngoing()) {
      showStep('Another recording is in progress. Stop it, then run again.');
      return;
    }

    setRunnerState('running');
    setAnalysis(null);
    clearLog();
    showStep('Preparing audio session...');
    appendLog('Starting speaker-to-mic loopback check.');

    try {
      const permission = await AudioManager.requestRecordingPermissions();
      if (permission !== 'Granted') {
        throw new Error('Recording permission was not granted.');
      }

      showStep('Playing beep pattern and analyzing microphone capture...');
      appendLog('Reference pattern scheduled. Recording microphone input.');

      runIORef.current = createLoopbackAudioIO();
      const result = await runLoopbackLatencyTest(runIORef.current);

      if (!isMountedRef.current) {
        return;
      }

      setAnalysis(result);
      appendLog(
        `Captured ${result.recordedWindowPlot.points.length > 0 ? 'microphone' : 'no'} waveform points for visualization.`
      );
      showStep(verdictMessages[result.scenario.status]);
      appendLog(`Test finished with status=${result.scenario.status}.`);
    } catch (error) {
      const message =
        error instanceof Error ? error.message : 'Unknown validation error';
      showStep(`Loopback check failed: ${message}`);
      appendLog(`Test failed: ${message}`);
    } finally {
      await releaseRunIO(runIORef);
      setRunnerState('idle');
    }
  };

  const resetTest = () => {
    setAnalysis(null);
    clearLog();
    showStep(READY_MESSAGE);
  };

  if (Platform.OS === 'web') {
    return (
      <Container centered>
        <TestUI.UnsupportedNotice
          title="Latency Validation"
          message="The speaker-to-mic loopback check requires native audio I/O and is not available on web."
        />
      </Container>
    );
  }

  const isRunning = runnerState === 'running';

  return (
    <TestUI.TestLayout
      description="Visual loopback analysis: generated beep pattern, microphone capture, aligned overlay, and correlation search."
      currentStep={currentStep}
      actions={[
        {
          title: 'Run Test',
          onPress: runTest,
          disabled: isRunning,
          minWidth: 120,
        },
        {
          title: 'Reset',
          onPress: resetTest,
          disabled: isRunning,
          minWidth: 90,
        },
      ]}
    >
      <LoopbackAnalysisPanel analysis={analysis} />
      <TestUI.LiveLog
        entries={liveLog}
        emptyMessage="Live log will appear here while the test runs."
      />
    </TestUI.TestLayout>
  );
};

export default LatencyValidation;
