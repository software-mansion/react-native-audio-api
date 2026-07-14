import React, { FC, useEffect, useRef, useState } from 'react';
import { Platform, ScrollView, StyleSheet } from 'react-native';
import { AudioManager } from 'react-native-audio-api';

import { Container } from '../../components';
import { audioContext, audioRecorder } from '../../singletons';
import { TestUI, type ControlAction } from '../../testComponents';
import { formatTimestamp } from './helpers';
import {
  cleanupLatencyValidation,
  runLatencyValidationSuite,
} from './latencyTests';
import LoopbackAnalysisPanel from './LoopbackAnalysisPanel';
import type { LoopbackAnalysis } from './types';

type RunnerState = 'idle' | 'running';

const LatencyValidation: FC = () => {
  const isMountedRef = useRef(true);
  const [runnerState, setRunnerState] = useState<RunnerState>('idle');
  const [currentStep, setCurrentStep] = useState(
    'Ready to run the speaker-to-mic loopback latency check.'
  );
  const [analysis, setAnalysis] = useState<LoopbackAnalysis | null>(null);
  const [liveLog, setLiveLog] = useState<string[]>([]);

  const appendLog = (message: string) => {
    if (!isMountedRef.current) {
      return;
    }

    setLiveLog((previous) => [
      ...previous,
      `${formatTimestamp(Date.now())} ${message}`,
    ]);
  };

  useEffect(() => {
    return () => {
      isMountedRef.current = false;
      cleanupLatencyValidation({ audioContext, audioRecorder }).catch((error) => {
        console.warn('LatencyValidation cleanup failed', error);
      });
    };
  }, []);

  const runTest = async () => {
    if (runnerState === 'running') {
      return;
    }

    setRunnerState('running');
    setAnalysis(null);
    setLiveLog([]);
    setCurrentStep('Preparing audio session...');
    appendLog('Starting speaker-to-mic loopback check.');

    try {
      const permission = await AudioManager.requestRecordingPermissions();
      if (permission !== 'Granted') {
        throw new Error('Recording permission was not granted.');
      }

      if (Platform.OS !== 'web') {
        AudioManager.setAudioSessionOptions({
          iosCategory: 'playAndRecord',
          // 'measurement' keeps the signal path raw (no AGC/EQ) so the loopback
          // delay is accurate, while running the mic+speaker for the shortest
          // possible window to keep the idle full-duplex buzz to a minimum.
          iosMode: 'measurement',
          iosOptions: ['defaultToSpeaker'],
        });
        await AudioManager.setAudioSessionActivity(true);
      }

      setCurrentStep('Playing beep pattern and analyzing microphone capture...');
      appendLog('Reference pattern scheduled. Recording microphone input.');

      const result = await runLatencyValidationSuite({
        audioContext,
        audioRecorder,
      });

      if (!isMountedRef.current) {
        return;
      }

      setAnalysis(result);
      appendLog(
        `Captured ${result.recordedWindowPlot.points.length > 0 ? 'microphone' : 'no'} waveform points for visualization.`
      );

      if (result.scenario.status === 'pass') {
        setCurrentStep(
          'Analysis complete: aligned waveform matches the reported latencies.'
        );
      } else if (result.scenario.status === 'skipped') {
        setCurrentStep(
          'Analysis complete: microphone capture did not contain a matchable beep pattern.'
        );
      } else {
        setCurrentStep(
          'Analysis complete: waveform alignment exists but latency delta is outside tolerance.'
        );
      }

      appendLog(`Test finished with status=${result.scenario.status}.`);
    } catch (error) {
      const message =
        error instanceof Error ? error.message : 'Unknown validation error';
      setCurrentStep(`Loopback check failed: ${message}`);
      appendLog(`Test failed: ${message}`);
    } finally {
      await cleanupLatencyValidation({ audioContext, audioRecorder });

      if (isMountedRef.current) {
        setRunnerState('idle');
      }
    }
  };

  const resetTest = async () => {
    if (runnerState === 'running') {
      return;
    }

    setAnalysis(null);
    setLiveLog([]);
    setCurrentStep('Ready to run the speaker-to-mic loopback latency check.');
    await cleanupLatencyValidation({ audioContext, audioRecorder });
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

  const controlActions: ControlAction[] = [
    {
      title: 'Run Test',
      onPress: () => {
        runTest();
      },
      disabled: runnerState === 'running',
      width: 120,
    },
    {
      title: 'Reset',
      onPress: () => {
        resetTest();
      },
      disabled: runnerState === 'running',
      width: 90,
    },
  ];

  return (
    <Container disablePadding>
      <TestUI.Header
        title="Latency Validation"
        subtitle="Visual loopback analysis: generated beep pattern, microphone capture, aligned overlay, and correlation search."
      />
      <TestUI.CurrentStepCard message={currentStep} />
      <TestUI.ControlBar actions={controlActions} />

      <ScrollView
        style={styles.scrollView}
        contentContainerStyle={styles.scrollContent}
      >
        <LoopbackAnalysisPanel analysis={analysis} />
        <TestUI.LiveLog
          entries={liveLog}
          emptyMessage="Live log will appear here while the test runs."
        />
      </ScrollView>
    </Container>
  );
};

const styles = StyleSheet.create({
  scrollContent: {
    gap: 12,
    paddingBottom: 32,
    paddingHorizontal: 18,
    paddingTop: 18,
  },
  scrollView: {
    flex: 1,
  },
});

export default LatencyValidation;
