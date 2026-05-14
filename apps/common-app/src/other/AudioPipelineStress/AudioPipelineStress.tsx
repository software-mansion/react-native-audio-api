import React, { FC, useEffect, useRef, useState } from 'react';
import { Platform, ScrollView, StyleSheet } from 'react-native';
import { AudioBuffer, AudioManager } from 'react-native-audio-api';

import { Container } from '../../components';
import {
  TestUI,
  type ControlAction,
  type ScenarioItem,
  type SummaryItem,
} from '../../testComponents';
import {
  DEFAULT_LOOP_COUNT,
  MIN_DECODED_DURATION_SECONDS,
  PLAYBACK_PROGRESS_TIMEOUT_MS,
  PLAYBACK_STALL_WINDOW_MS,
  POSITION_POLL_INTERVAL_MS,
  RECORDING_CALLBACK_TIMEOUT_MS,
  RECORDING_STALL_WINDOW_MS,
  SHORT_PLAYBACK_MS,
  SHORT_RECORDING_MS,
} from './constants';
import StressResourceOwner from './StressResourceOwner';
import StopRequestedError from './StopRequestedError';
import { activatePlaybackSession, activateRecordingSession } from './audioSessions';
import {
  formatDurationMs,
  formatPlaybackProgressStats,
  formatTimestamp,
  getPlaybackProgressThreshold,
  serializeUnknownError,
  sleep,
  waitForCondition,
} from './helpers';
import staticAsset from '../../examples/AudioFile/voice-sample-landing.mp3';
import type {
  PlaybackProgressStats,
  RecordingCapture,
  RunnerState,
  ScenarioId,
  ScenarioResult,
  ScenarioStatus,
  StepResult,
} from './types';

const AudioPipelineStress: FC = () => {
  const resourcesRef = useRef(new StressResourceOwner(staticAsset));
  const stopRequestedRef = useRef(false);
  const runIdRef = useRef(0);
  const isMountedRef = useRef(true);

  const [runnerState, setRunnerState] = useState<RunnerState>('idle');
  const [currentStep, setCurrentStep] = useState<string>(
    'Ready to run the audio pipeline stress suite.'
  );
  const [scenarioResults, setScenarioResults] = useState<ScenarioResult[]>([]);
  const [liveLog, setLiveLog] = useState<string[]>([]);

  useEffect(() => {
    const resources = resourcesRef.current;

    return () => {
      isMountedRef.current = false;
      stopRequestedRef.current = true;
      resources.cleanup().catch((error) => {
        console.warn('AudioPipelineStress cleanup failed', error);
      });
    };
  }, []);

  const appendLog = (message: string) => {
    if (!isMountedRef.current) {
      return;
    }

    setLiveLog((previous) => [
      ...previous,
      `${formatTimestamp(Date.now())} ${message}`,
    ]);
  };

  const setVisibleStep = (message: string) => {
    if (!isMountedRef.current) {
      return;
    }

    setCurrentStep(message);
  };

  const requestStop = () => {
    stopRequestedRef.current = true;
    setRunnerState((state) => (state === 'running' ? 'stopping' : state));
    appendLog('Stop requested.');
  };

  const ensureCanContinue = () => {
    if (stopRequestedRef.current) {
      throw new StopRequestedError();
    }
  };

  const hardResetResources = async (reason: string) => {
    appendLog(`Hard reset: ${reason}`);
    await resourcesRef.current.recreate();
  };

  const runStep = async (
    scenarioLabel: string,
    stepResults: StepResult[],
    id: string,
    message: string,
    action: () => Promise<void>
  ) => {
    ensureCanContinue();
    setVisibleStep(`${scenarioLabel}: ${message}`);
    appendLog(`[${scenarioLabel}] ${message}`);

    const startedAt = Date.now();

    try {
      await action();

      stepResults.push({
        id,
        message,
        status: 'pass',
        startedAt,
        finishedAt: Date.now(),
      });
    } catch (error) {
      const serializedError = serializeUnknownError(
        error,
        `${scenarioLabel} / ${id}`
      );

      stepResults.push({
        id,
        message,
        status: error instanceof StopRequestedError ? 'skipped' : 'fail',
        startedAt,
        finishedAt: Date.now(),
        details: serializedError.details,
      });

      appendLog(`[${scenarioLabel}] ${serializedError.headline}`);
      throw error;
    }
  };

  const addInfoStep = (
    stepResults: StepResult[],
    id: string,
    message: string,
    details?: string
  ) => {
    const now = Date.now();
    stepResults.push({
      id,
      message,
      status: 'info',
      startedAt: now,
      finishedAt: now,
      details,
    });
  };

  const waitForPlaybackProgress = async (
    minimumSeconds: number,
    contextLabel: string
  ): Promise<PlaybackProgressStats> => {
    const { context, playback } = resourcesRef.current.getReadyResources();
    const startedAt = Date.now();
    const positionSamples: number[] = [];
    const engineSamples: number[] = [];
    const engineStartTimeSeconds = context.currentTime;
    let maxObserved = 0;
    let lastSnapshot = playback.snapshot();
    let lastEngineTimeSeconds = engineStartTimeSeconds;

    while (Date.now() - startedAt < PLAYBACK_PROGRESS_TIMEOUT_MS) {
      ensureCanContinue();
      lastSnapshot = playback.snapshot();
      lastEngineTimeSeconds = context.currentTime;
      positionSamples.push(Number(lastSnapshot.lastPositionSeconds.toFixed(3)));
      engineSamples.push(Number(lastEngineTimeSeconds.toFixed(3)));
      maxObserved = Math.max(maxObserved, lastSnapshot.lastPositionSeconds);

      if (lastSnapshot.lastPositionSeconds >= minimumSeconds) {
        return {
          engineEndTimeSeconds: lastEngineTimeSeconds,
          engineDeltaSeconds: lastEngineTimeSeconds - engineStartTimeSeconds,
          engineSamples,
          engineStartTimeSeconds,
          positionMaxObservedSeconds: maxObserved,
          positionSamples,
          thresholdSeconds: minimumSeconds,
        };
      }

      await sleep(POSITION_POLL_INTERVAL_MS);
    }

    const recorder = resourcesRef.current.recorder;
    const stats: PlaybackProgressStats = {
      engineEndTimeSeconds: lastEngineTimeSeconds,
      engineDeltaSeconds: lastEngineTimeSeconds - engineStartTimeSeconds,
      engineSamples,
      engineStartTimeSeconds,
      positionMaxObservedSeconds: maxObserved,
      positionSamples,
      thresholdSeconds: minimumSeconds,
    };
    const diagnosis =
      stats.engineDeltaSeconds < 0.05
        ? 'engine-clock-stalled'
        : 'engine-running-node-stalled';

    throw new Error(
      [
        `${contextLabel}: playback progress check failed`,
        `diagnosis=${diagnosis}`,
        `target=${minimumSeconds.toFixed(2)}s`,
        `maxObserved=${maxObserved.toFixed(3)}s`,
        `engineStart=${engineStartTimeSeconds.toFixed(3)}s`,
        `engineEnd=${lastEngineTimeSeconds.toFixed(3)}s`,
        `engineDelta=${stats.engineDeltaSeconds.toFixed(3)}s`,
        `recentPositionSamples=[${positionSamples.slice(-8).join(', ')}]`,
        `recentEngineSamples=[${engineSamples.slice(-8).join(', ')}]`,
        `contextState=${context.state}`,
        `playbackActive=${lastSnapshot.isActive}`,
        `playbackEnded=${lastSnapshot.ended}`,
        `recorderRecording=${recorder?.isRecording() ?? false}`,
        `recorderPaused=${recorder?.isPaused() ?? false}`,
        `recorderCallbacks=${resourcesRef.current.callbackCount}`,
        `lastRecorderError=${resourcesRef.current.lastRecorderError ?? 'none'}`,
      ].join(', ')
    );
  };

  const waitForPlaybackToStall = async () => {
    const { context, playback } = resourcesRef.current.getReadyResources();

    const engineStartTimeSeconds = context.currentTime;
    const before = playback.snapshot().lastPositionSeconds;
    await sleep(PLAYBACK_STALL_WINDOW_MS);
    const after = playback.snapshot().lastPositionSeconds;
    const engineEndTimeSeconds = context.currentTime;
    const engineDeltaSeconds = engineEndTimeSeconds - engineStartTimeSeconds;

    if (after - before > 0.1) {
      throw new Error(
        `Playback still advanced after deactivation (${before.toFixed(2)}s -> ${after.toFixed(2)}s), engineDelta=${engineDeltaSeconds.toFixed(3)}s`
      );
    }
  };

  const waitForRecordingCallbacks = async (minimumCount: number) => {
    await waitForCondition(
      () => resourcesRef.current.callbackCount >= minimumCount,
      RECORDING_CALLBACK_TIMEOUT_MS,
      `Recorder did not emit ${minimumCount} callback buffer(s)`
    );
  };

  const waitForRecordingToStall = async () => {
    const beforeCount = resourcesRef.current.callbackCount;
    await sleep(RECORDING_STALL_WINDOW_MS);
    const afterCount = resourcesRef.current.callbackCount;
    const recorder = resourcesRef.current.recorder;

    if (afterCount > beforeCount) {
      throw new Error(
        `Recorder callback count kept growing after deactivation (${beforeCount} -> ${afterCount})`
      );
    }

    if (recorder && recorder.isRecording() && !recorder.isPaused()) {
      throw new Error(
        'Recorder still reports an active recording after deactivation'
      );
    }
  };

  const performCleanRecording = async (
    fileNameOverride: string
  ): Promise<RecordingCapture> => {
    await activateRecordingSession();

    resourcesRef.current.configureRecorderTap();
    resourcesRef.current.startRecording(fileNameOverride);
    await waitForRecordingCallbacks(1);
    await sleep(SHORT_RECORDING_MS);

    const capture = await resourcesRef.current.stopRecordingAndDecode();

    if (capture.fileDurationSeconds < MIN_DECODED_DURATION_SECONDS) {
      throw new Error(
        `Recorded file duration is too short: ${capture.fileDurationSeconds.toFixed(3)}s`
      );
    }

    await AudioManager.setAudioSessionActivity(false);
    return capture;
  };

  const performCleanPlayback = async (
    buffer: AudioBuffer,
    durationSeconds: number = 1.4,
    contextLabel: string = 'clean playback'
  ): Promise<PlaybackProgressStats> => {
    const { playback } = resourcesRef.current.getReadyResources();
    const expectedPlaybackWindow = Math.min(buffer.duration, durationSeconds);
    const progressThreshold = getPlaybackProgressThreshold(
      expectedPlaybackWindow
    );

    await activatePlaybackSession();
    await playback.play(buffer, durationSeconds);
    const playbackStats = await waitForPlaybackProgress(
      progressThreshold,
      contextLabel
    );
    await sleep(SHORT_PLAYBACK_MS);
    playback.stop();
    await AudioManager.setAudioSessionActivity(false);
    return playbackStats;
  };

  const performCleanRecordPlaybackCycle = async (label: string) => {
    const capture = await performCleanRecording(`${label}-${Date.now()}`);
    await performCleanPlayback(
      capture.decodedBuffer,
      Math.min(capture.decodedBuffer.duration, 1.4),
      `${label}: playback after recording`
    );
  };

  const runScenario = async (
    scenarioId: ScenarioId,
    label: string,
    action: (steps: StepResult[]) => Promise<void>
  ) => {
    const startedAt = Date.now();
    const stepResults: StepResult[] = [];
    let status: ScenarioStatus = 'pass';
    let errorMessage: string | undefined;

    appendLog(`Scenario start: ${label}`);

    try {
      await action(stepResults);
    } catch (error) {
      if (error instanceof StopRequestedError) {
        status = 'skipped';
        errorMessage = error.message;
      } else {
        status = 'fail';
        errorMessage = serializeUnknownError(error, label).details;
      }
    }

    const finishedAt = Date.now();
    const result: ScenarioResult = {
      scenarioId,
      label,
      status,
      startedAt,
      finishedAt,
      steps: stepResults,
      error: errorMessage,
    };

    if (status === 'fail' && errorMessage) {
      addInfoStep(
        stepResults,
        `${scenarioId}-failure-summary`,
        'Scenario failed',
        errorMessage
      );
      appendLog(
        `Scenario failed: ${label} (${errorMessage.split('\n')[0] ?? errorMessage})`
      );
    } else {
      appendLog(`Scenario ${status}: ${label}`);
    }

    if (isMountedRef.current) {
      setScenarioResults((previous) => [...previous, result]);
    }

    if (status === 'fail') {
      await hardResetResources(`${label} failed`);
    }

    if (status === 'skipped') {
      throw new StopRequestedError();
    }
  };

  const runSuite = async () => {
    if (runnerState === 'running' || runnerState === 'stopping') {
      return;
    }

    runIdRef.current += 1;
    const currentRunId = runIdRef.current;
    stopRequestedRef.current = false;

    setRunnerState('running');
    setScenarioResults([]);
    setLiveLog([]);
    setVisibleStep('Preparing audio pipeline resources...');

    try {
      const permissionStatus = await AudioManager.requestRecordingPermissions();

      if (permissionStatus !== 'Granted') {
        throw new Error(
          `Microphone permission is required to run the stress suite (status=${permissionStatus})`
        );
      }

      await resourcesRef.current.recreate();

      await runScenario('playback_warmup', 'Playback Warmup', async (steps) => {
        await runStep(
          'Playback Warmup',
          steps,
          'activate-playback',
          'Activate playback session',
          async () => {
            await activatePlaybackSession();
          }
        );
        await runStep(
          'Playback Warmup',
          steps,
          'play-asset',
          'Play bundled asset and observe progress',
          async () => {
            const { assetBuffer, playback } =
              resourcesRef.current.getReadyResources();
            const progressThreshold = getPlaybackProgressThreshold(1.4);
            await playback.play(assetBuffer, 1.4);
            const playbackStats = await waitForPlaybackProgress(
              progressThreshold,
              'Playback Warmup'
            );
            await sleep(SHORT_PLAYBACK_MS);
            playback.stop();
            addInfoStep(
              steps,
              'playback-warmup-engine-timing',
              'Playback engine timing',
              formatPlaybackProgressStats(playbackStats)
            );
          }
        );
        await runStep(
          'Playback Warmup',
          steps,
          'deactivate-playback',
          'Deactivate playback session',
          async () => {
            await AudioManager.setAudioSessionActivity(false);
          }
        );
      });

      await runScenario('record_warmup', 'Record Warmup', async (steps) => {
        await runStep(
          'Record Warmup',
          steps,
          'record-and-decode',
          'Record briefly, then decode output',
          async () => {
            const capture = await performCleanRecording(
              `record-warmup-${Date.now()}`
            );
            addInfoStep(
              steps,
              'recorded-file',
              'Decoded recording metadata',
              `${capture.path} (${capture.fileDurationSeconds.toFixed(2)}s)`
            );
          }
        );
      });

      await runScenario(
        'record_to_playback_loop',
        'Record To Playback Loop',
        async (steps) => {
          for (let index = 0; index < DEFAULT_LOOP_COUNT; index += 1) {
            const cycle = index + 1;
            await runStep(
              'Record To Playback Loop',
              steps,
              `cycle-${cycle}`,
              `Cycle ${cycle}: record, decode, and play recorded audio`,
              async () => {
                const capture = await performCleanRecording(
                  `record-to-playback-${cycle}-${Date.now()}`
                );

                const playbackStats = await performCleanPlayback(
                  capture.decodedBuffer,
                  Math.min(capture.decodedBuffer.duration, 1.4),
                  `Record To Playback Loop cycle ${cycle}`
                );
                addInfoStep(
                  steps,
                  `cycle-${cycle}-recorded-buffer`,
                  `Cycle ${cycle} recorded file metadata`,
                  `fileDuration=${capture.fileDurationSeconds.toFixed(3)}s, decodedDuration=${capture.decodedBuffer.duration.toFixed(3)}s, decodedLength=${capture.decodedBuffer.length}, path=${capture.path}`
                );
                addInfoStep(
                  steps,
                  `cycle-${cycle}-playback-engine-timing`,
                  `Cycle ${cycle} playback engine timing`,
                  formatPlaybackProgressStats(playbackStats)
                );
              }
            );
          }
        }
      );

      await runScenario(
        'playback_to_record_loop',
        'Playback To Record Loop',
        async (steps) => {
          for (let index = 0; index < DEFAULT_LOOP_COUNT; index += 1) {
            const cycle = index + 1;
            await runStep(
              'Playback To Record Loop',
              steps,
              `cycle-${cycle}`,
              `Cycle ${cycle}: play bundled asset, then record and decode`,
              async () => {
                const { assetBuffer } =
                  resourcesRef.current.getReadyResources();
                const playbackStats = await performCleanPlayback(
                  assetBuffer,
                  1.4,
                  `Playback To Record Loop cycle ${cycle}`
                );
                addInfoStep(
                  steps,
                  `cycle-${cycle}-pre-record-playback-engine-timing`,
                  `Cycle ${cycle} pre-record playback engine timing`,
                  formatPlaybackProgressStats(playbackStats)
                );
                await performCleanRecording(
                  `playback-to-record-${cycle}-${Date.now()}`
                );
              }
            );
          }
        }
      );

      await runScenario(
        'playback_session_deactivation_mid_run',
        'Playback Session Deactivation Mid Run',
        async (steps) => {
          await runStep(
            'Playback Session Deactivation Mid Run',
            steps,
            'start-playback',
            'Start playback and wait for initial progress',
            async () => {
              const { assetBuffer, playback } =
                resourcesRef.current.getReadyResources();
              const progressThreshold = getPlaybackProgressThreshold(2.2);
              await activatePlaybackSession();
              await playback.play(assetBuffer, 2.2);
              const playbackStats = await waitForPlaybackProgress(
                progressThreshold,
                'Playback Session Deactivation Mid Run'
              );
              addInfoStep(
                steps,
                'pre-deactivation-playback-engine-timing',
                'Playback engine timing before session deactivation',
                formatPlaybackProgressStats(playbackStats)
              );
            }
          );
          await runStep(
            'Playback Session Deactivation Mid Run',
            steps,
            'deactivate-mid-playback',
            'Deactivate session without pausing playback first',
            async () => {
              await AudioManager.setAudioSessionActivity(false);
              await waitForPlaybackToStall();
            }
          );
          await runStep(
            'Playback Session Deactivation Mid Run',
            steps,
            'reactivate-playback',
            'Reactivate playback and confirm fresh playback still works',
            async () => {
              const { assetBuffer, playback } =
                resourcesRef.current.getReadyResources();
              playback.stop();
              const playbackStats = await performCleanPlayback(
                assetBuffer,
                1.2,
                'Playback Session Deactivation Recovery'
              );
              addInfoStep(
                steps,
                'post-deactivation-recovery-engine-timing',
                'Playback engine timing after recovery',
                formatPlaybackProgressStats(playbackStats)
              );
            }
          );
        }
      );

      await runScenario(
        'record_session_deactivation_mid_run',
        'Record Session Deactivation Mid Run',
        async (steps) => {
          await runStep(
            'Record Session Deactivation Mid Run',
            steps,
            'start-recording',
            'Start recording and wait for callback data',
            async () => {
              await activateRecordingSession();
              resourcesRef.current.configureRecorderTap();
              resourcesRef.current.startRecording(
                `record-deactivation-${Date.now()}`
              );
              await waitForRecordingCallbacks(1);
            }
          );
          await runStep(
            'Record Session Deactivation Mid Run',
            steps,
            'deactivate-mid-recording',
            'Deactivate session without stopping recording first',
            async () => {
              await AudioManager.setAudioSessionActivity(false);
              await waitForRecordingToStall();
              const recorder = resourcesRef.current.recorder;

              addInfoStep(
                steps,
                'post-deactivation-paused-state',
                'Recorder paused state after session deactivation',
                `isPaused=${recorder?.isPaused() ?? false}`
              );
            }
          );
          await runStep(
            'Record Session Deactivation Mid Run',
            steps,
            'stop-after-deactivation',
            'Attempt stop after deactivation, then recreate resources',
            async () => {
              const recorder = resourcesRef.current.recorder;

              if (!recorder) {
                throw new Error('Recorder is not ready');
              }

              const result = recorder.stop();
              recorder.clearOnAudioReady();

              if (result.status === 'error') {
                addInfoStep(
                  steps,
                  'stop-after-deactivation-result',
                  'Recorder stop returned an error after deactivation',
                  result.message
                );
              }

              await hardResetResources('recover after recording deactivation');
            }
          );
          await runStep(
            'Record Session Deactivation Mid Run',
            steps,
            'clean-recovery-cycle',
            'Run one clean record and decode cycle after recovery',
            async () => {
              await performCleanRecording(`post-record-recovery-${Date.now()}`);
            }
          );
        }
      );

      await runScenario(
        'wrong_category_then_recover',
        'Wrong Category Then Recover',
        async (steps) => {
          await runStep(
            'Wrong Category Then Recover',
            steps,
            'attempt-wrong-category-record',
            'Attempt recording while the session is configured for playback',
            async () => {
              AudioManager.setAudioSessionOptions({
                iosCategory: 'playback',
                iosMode: 'default',
                iosOptions: [],
              });

              const active = await AudioManager.setAudioSessionActivity(true);

              if (!active) {
                throw new Error('Failed to activate playback session');
              }

              resourcesRef.current.configureRecorderTap();
              const result = resourcesRef.current.tryStartRecording(
                `wrong-category-${Date.now()}`
              );

              if (result.status === 'success') {
                throw new Error(
                  'Recording unexpectedly started under playback-only session settings'
                );
              }

              addInfoStep(
                steps,
                'expected-recording-failure',
                'Recorder rejected the wrong category as expected',
                result.message
              );

              resourcesRef.current.recorder?.clearOnAudioReady();
              await AudioManager.setAudioSessionActivity(false);
            }
          );
          await runStep(
            'Wrong Category Then Recover',
            steps,
            'clean-recovery-record',
            'Switch back to playAndRecord and confirm clean recording works',
            async () => {
              await performCleanRecording(
                `wrong-category-recovery-${Date.now()}`
              );
            }
          );
        }
      );

      await runScenario(
        'final_clean_cycle',
        'Final Clean Cycle',
        async (steps) => {
          await runStep(
            'Final Clean Cycle',
            steps,
            'final-record-playback-cycle',
            'Run one final clean record and playback cycle',
            async () => {
              await performCleanRecordPlaybackCycle('final-clean-cycle');
            }
          );
        }
      );

      if (!stopRequestedRef.current) {
        appendLog('Suite completed.');
        setVisibleStep('Suite completed.');
      }
    } catch (error) {
      if (error instanceof StopRequestedError) {
        appendLog('Suite stopped before completion.');
        setVisibleStep('Suite stopped.');
      } else {
        const serializedError = serializeUnknownError(error, 'Suite');
        appendLog(`Suite aborted: ${serializedError.headline}`);
        setVisibleStep(serializedError.headline);
      }
    } finally {
      await resourcesRef.current.cleanup();

      if (!isMountedRef.current || currentRunId !== runIdRef.current) {
        return;
      }

      setRunnerState(stopRequestedRef.current ? 'finished' : 'finished');

      if (!stopRequestedRef.current && scenarioResults.length === 0) {
        setVisibleStep('Suite finished.');
      }
    }
  };

  const resetSuite = async () => {
    if (runnerState === 'running' || runnerState === 'stopping') {
      return;
    }

    setVisibleStep('Resetting resources...');
    appendLog('Resetting screen state.');
    setScenarioResults([]);
    setLiveLog([]);
    await resourcesRef.current.cleanup();
    setVisibleStep('Ready to run the audio pipeline stress suite.');
    setRunnerState('idle');
  };

  if (Platform.OS !== 'ios') {
    return (
      <Container centered>
        <TestUI.UnsupportedNotice
          title="Audio Pipeline Stress"
          message="This screen is iOS-only because it validates the shared iOS audio session and engine pipeline."
        />
      </Container>
    );
  }

  const passedScenarios = scenarioResults.filter(
    (result) => result.status === 'pass'
  ).length;
  const failedScenarios = scenarioResults.filter(
    (result) => result.status === 'fail'
  ).length;
  const skippedScenarios = scenarioResults.filter(
    (result) => result.status === 'skipped'
  ).length;
  const summaryItems: SummaryItem[] = [
    { label: 'State', value: runnerState },
    { label: 'Passed', value: String(passedScenarios) },
    { label: 'Failed', value: String(failedScenarios) },
    { label: 'Skipped', value: String(skippedScenarios) },
  ];
  const controlActions: ControlAction[] = [
    {
      title: 'Run Suite',
      onPress: () => {
        runSuite();
      },
      disabled: runnerState === 'running' || runnerState === 'stopping',
      width: 120,
    },
    {
      title: 'Stop',
      onPress: requestStop,
      disabled: runnerState !== 'running',
      width: 90,
    },
    {
      title: 'Reset',
      onPress: () => {
        resetSuite();
      },
      disabled: runnerState === 'running' || runnerState === 'stopping',
      width: 90,
    },
  ];
  const scenarioItems: ScenarioItem[] = scenarioResults.map((scenario) => ({
    id: `${scenario.scenarioId}-${scenario.startedAt}`,
    title: scenario.label,
    status: scenario.status,
    durationLabel: formatDurationMs(scenario.finishedAt - scenario.startedAt),
    steps: scenario.steps.map((step) => ({
      id: `${scenario.scenarioId}-${step.id}`,
      message: step.message,
      status: step.status,
      details: step.details,
    })),
  }));

  return (
    <Container disablePadding>
      <TestUI.Header
        title="Audio Pipeline Stress"
        subtitle="Automated iOS playback and recording pipeline stress suite."
      />
      <TestUI.Summary items={summaryItems} />
      <TestUI.CurrentStepCard message={currentStep} />
      <TestUI.ControlBar actions={controlActions} />

      <ScrollView
        style={styles.scrollView}
        contentContainerStyle={styles.scrollContent}
      >
        <TestUI.ScenarioResults
          scenarios={scenarioItems}
          emptyMessage="No scenario results yet."
        />
        <TestUI.LiveLog
          entries={liveLog}
          emptyMessage="Live log will appear here while the suite runs."
        />
      </ScrollView>
    </Container>
  );
};

const styles = StyleSheet.create({
  scrollContent: {
    gap: 12,
    paddingHorizontal: 18,
    paddingTop: 18,
    paddingBottom: 32,
  },
  scrollView: {
    flex: 1,
  },
});

export default AudioPipelineStress;
