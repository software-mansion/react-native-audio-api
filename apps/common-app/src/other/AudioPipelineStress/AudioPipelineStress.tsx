import React, { FC, useEffect, useRef, useState } from 'react';
import { Platform } from 'react-native';
import { AudioBuffer, AudioManager } from 'react-native-audio-api';

import { Container } from '../../components';
import * as TestUI from '../../testComponents';
import type {
  ControlAction,
  ScenarioItem,
  ScenarioStatus,
  StepItem,
} from '../../testComponents';
import { sleep } from '../sleep';
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
import {
  activatePlaybackSession,
  activateRecordingSession,
} from './audioSessions';
import {
  formatDurationMs,
  formatPlaybackProgressStats,
  getPlaybackProgressThreshold,
  serializeUnknownError,
  waitForCondition,
} from './helpers';
import staticAsset from '../../examples/AudioPlayback/voice-sample-landing.mp3';
import type {
  PlaybackProgressStats,
  RecordingCapture,
  RunnerState,
} from './types';

const READY_MESSAGE = 'Ready to run the audio pipeline stress suite.';

interface ScenarioSteps {
  /** Runs one step; a failure is recorded and aborts the scenario. */
  run: (message: string, action: () => Promise<unknown>) => Promise<void>;
  /** Records diagnostics that are neither a pass nor a failure. */
  info: (message: string, details?: string) => void;
}

const AudioPipelineStress: FC = () => {
  const resourcesRef = useRef(new StressResourceOwner(staticAsset));
  const stopRequestedRef = useRef(false);
  const {
    runnerState,
    setRunnerState,
    currentStep,
    showStep,
    liveLog,
    appendLog,
    clearLog,
  } = TestUI.useRunLog<RunnerState>('idle', READY_MESSAGE);
  const [scenarioResults, setScenarioResults] = useState<ScenarioItem[]>([]);

  useEffect(() => {
    const resources = resourcesRef.current;

    return () => {
      stopRequestedRef.current = true;
      resources.cleanup().catch((error) => {
        console.warn('AudioPipelineStress cleanup failed', error);
      });
    };
  }, []);

  const isBusy = runnerState === 'running' || runnerState === 'stopping';

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

    const collectStats = (): PlaybackProgressStats => ({
      engineEndTimeSeconds: lastEngineTimeSeconds,
      engineDeltaSeconds: lastEngineTimeSeconds - engineStartTimeSeconds,
      engineSamples,
      engineStartTimeSeconds,
      positionMaxObservedSeconds: maxObserved,
      positionSamples,
      thresholdSeconds: minimumSeconds,
    });

    while (Date.now() - startedAt < PLAYBACK_PROGRESS_TIMEOUT_MS) {
      ensureCanContinue();
      lastSnapshot = playback.snapshot();
      lastEngineTimeSeconds = context.currentTime;
      positionSamples.push(lastSnapshot.lastPositionSeconds);
      engineSamples.push(lastEngineTimeSeconds);
      maxObserved = Math.max(maxObserved, lastSnapshot.lastPositionSeconds);

      if (lastSnapshot.lastPositionSeconds >= minimumSeconds) {
        return collectStats();
      }

      await sleep(POSITION_POLL_INTERVAL_MS);
    }

    const { recorder, callbackCount, lastRecorderError } = resourcesRef.current;
    throw new Error(
      [
        `${contextLabel}: playback progress check failed`,
        formatPlaybackProgressStats(collectStats()),
        `contextState=${context.state}`,
        `playbackActive=${lastSnapshot.isActive}`,
        `playbackEnded=${lastSnapshot.ended}`,
        `recorderRecording=${recorder?.isRecording() ?? false}`,
        `recorderPaused=${recorder?.isPaused() ?? false}`,
        `recorderCallbacks=${callbackCount}`,
        `lastRecorderError=${lastRecorderError ?? 'none'}`,
      ].join(', ')
    );
  };

  const waitForPlaybackToStall = async () => {
    const { context, playback } = resourcesRef.current.getReadyResources();

    const engineStartTimeSeconds = context.currentTime;
    const before = playback.snapshot().lastPositionSeconds;
    await sleep(PLAYBACK_STALL_WINDOW_MS);
    const after = playback.snapshot().lastPositionSeconds;
    const engineDeltaSeconds = context.currentTime - engineStartTimeSeconds;

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

  const performCleanRecording = async (): Promise<RecordingCapture> => {
    await activateRecordingSession();

    resourcesRef.current.configureRecorderTap();
    await resourcesRef.current.startRecording();
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
    durationSeconds: number,
    contextLabel: string
  ): Promise<PlaybackProgressStats> => {
    const { playback } = resourcesRef.current.getReadyResources();
    const progressThreshold = getPlaybackProgressThreshold(
      Math.min(buffer.duration, durationSeconds)
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

  const runScenario = async (
    label: string,
    action: (steps: ScenarioSteps) => Promise<void>
  ) => {
    const startedAt = Date.now();
    const stepItems: StepItem[] = [];
    const addStep = (step: Omit<StepItem, 'id'>) => {
      stepItems.push({ id: String(stepItems.length), ...step });
    };
    const steps: ScenarioSteps = {
      run: async (message, stepAction) => {
        ensureCanContinue();
        showStep(`${label}: ${message}`);
        appendLog(`[${label}] ${message}`);

        try {
          await stepAction();
          addStep({ message, status: 'pass' });
        } catch (error) {
          const serializedError = serializeUnknownError(error, label);
          addStep({
            message,
            status: error instanceof StopRequestedError ? 'skipped' : 'fail',
            details: serializedError.details,
          });
          appendLog(`[${label}] ${serializedError.headline}`);
          throw error;
        }
      },
      info: (message, details) => addStep({ message, status: 'info', details }),
    };

    let status: ScenarioStatus = 'pass';
    appendLog(`Scenario start: ${label}`);

    try {
      await action(steps);
      appendLog(`Scenario pass: ${label}`);
    } catch (error) {
      if (error instanceof StopRequestedError) {
        status = 'skipped';
        appendLog(`Scenario skipped: ${label}`);
      } else {
        status = 'fail';
        const { headline, details } = serializeUnknownError(error, label);
        steps.info('Scenario failed', details);
        appendLog(`Scenario failed: ${headline}`);
      }
    }

    setScenarioResults((previous) => [
      ...previous,
      {
        id: `${label}-${startedAt}`,
        title: label,
        status,
        durationLabel: formatDurationMs(Date.now() - startedAt),
        steps: stepItems,
      },
    ]);

    if (status === 'fail') {
      await hardResetResources(`${label} failed`);
    }

    if (status === 'skipped') {
      throw new StopRequestedError();
    }
  };

  const runSuite = async () => {
    if (isBusy) {
      return;
    }

    stopRequestedRef.current = false;
    setRunnerState('running');
    setScenarioResults([]);
    clearLog();
    showStep('Preparing audio pipeline resources...');

    try {
      const permissionStatus = await AudioManager.requestRecordingPermissions();

      if (permissionStatus !== 'Granted') {
        throw new Error(
          `Microphone permission is required to run the stress suite (status=${permissionStatus})`
        );
      }

      await resourcesRef.current.recreate();

      await runScenario('Playback Warmup', async ({ run, info }) => {
        await run('Activate playback session', activatePlaybackSession);
        await run('Play bundled asset and observe progress', async () => {
          const { assetBuffer, playback } =
            resourcesRef.current.getReadyResources();
          await playback.play(assetBuffer, 1.4);
          const playbackStats = await waitForPlaybackProgress(
            getPlaybackProgressThreshold(1.4),
            'Playback Warmup'
          );
          await sleep(SHORT_PLAYBACK_MS);
          playback.stop();
          info(
            'Playback engine timing',
            formatPlaybackProgressStats(playbackStats)
          );
        });
        await run('Deactivate playback session', () =>
          AudioManager.setAudioSessionActivity(false)
        );
      });

      await runScenario('Record Warmup', async ({ run, info }) => {
        await run('Record briefly, then decode output', async () => {
          const capture = await performCleanRecording();
          info(
            'Decoded recording metadata',
            `${capture.path} (${capture.fileDurationSeconds.toFixed(2)}s)`
          );
        });
      });

      await runScenario('Record To Playback Loop', async ({ run, info }) => {
        for (let cycle = 1; cycle <= DEFAULT_LOOP_COUNT; cycle += 1) {
          await run(
            `Cycle ${cycle}: record, decode, and play recorded audio`,
            async () => {
              const capture = await performCleanRecording();
              const playbackStats = await performCleanPlayback(
                capture.decodedBuffer,
                1.4,
                `Record To Playback Loop cycle ${cycle}`
              );
              info(
                `Cycle ${cycle} recorded file metadata`,
                `fileDuration=${capture.fileDurationSeconds.toFixed(3)}s, decodedDuration=${capture.decodedBuffer.duration.toFixed(3)}s, decodedLength=${capture.decodedBuffer.length}, path=${capture.path}`
              );
              info(
                `Cycle ${cycle} playback engine timing`,
                formatPlaybackProgressStats(playbackStats)
              );
            }
          );
        }
      });

      await runScenario('Playback To Record Loop', async ({ run, info }) => {
        for (let cycle = 1; cycle <= DEFAULT_LOOP_COUNT; cycle += 1) {
          await run(
            `Cycle ${cycle}: play bundled asset, then record and decode`,
            async () => {
              const { assetBuffer } = resourcesRef.current.getReadyResources();
              const playbackStats = await performCleanPlayback(
                assetBuffer,
                1.4,
                `Playback To Record Loop cycle ${cycle}`
              );
              info(
                `Cycle ${cycle} pre-record playback engine timing`,
                formatPlaybackProgressStats(playbackStats)
              );
              await performCleanRecording();
            }
          );
        }
      });

      await runScenario(
        'Playback Session Deactivation Mid Run',
        async ({ run, info }) => {
          await run(
            'Start playback and wait for initial progress',
            async () => {
              const { assetBuffer, playback } =
                resourcesRef.current.getReadyResources();
              await activatePlaybackSession();
              await playback.play(assetBuffer, 2.2);
              const playbackStats = await waitForPlaybackProgress(
                getPlaybackProgressThreshold(2.2),
                'Playback Session Deactivation Mid Run'
              );
              info(
                'Playback engine timing before session deactivation',
                formatPlaybackProgressStats(playbackStats)
              );
            }
          );
          await run(
            'Deactivate session without pausing playback first',
            async () => {
              await AudioManager.setAudioSessionActivity(false);
              await waitForPlaybackToStall();
            }
          );
          await run(
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
              info(
                'Playback engine timing after recovery',
                formatPlaybackProgressStats(playbackStats)
              );
            }
          );
        }
      );

      await runScenario(
        'Record Session Deactivation Mid Run',
        async ({ run, info }) => {
          await run('Start recording and wait for callback data', async () => {
            await activateRecordingSession();
            resourcesRef.current.configureRecorderTap();
            await resourcesRef.current.startRecording();
            await waitForRecordingCallbacks(1);
          });
          await run(
            'Deactivate session without stopping recording first',
            async () => {
              await AudioManager.setAudioSessionActivity(false);
              await waitForRecordingToStall();
              info(
                'Recorder paused state after session deactivation',
                `isPaused=${resourcesRef.current.recorder?.isPaused() ?? false}`
              );
            }
          );
          await run(
            'Attempt stop after deactivation, then recreate resources',
            async () => {
              const { recorder } = resourcesRef.current.getReadyResources();

              try {
                await recorder.stop();
              } catch (error) {
                info(
                  'Recorder stop rejected after deactivation',
                  error instanceof Error ? error.message : String(error)
                );
              } finally {
                recorder.clearOnAudioReady();
              }

              await hardResetResources('recover after recording deactivation');
            }
          );
          await run(
            'Run one clean record and decode cycle after recovery',
            performCleanRecording
          );
        }
      );

      await runScenario(
        'Wrong Category Then Recover',
        async ({ run, info }) => {
          await run(
            'Attempt recording while the session is configured for playback',
            async () => {
              await activatePlaybackSession();
              resourcesRef.current.configureRecorderTap();

              let startError: unknown;
              try {
                await resourcesRef.current.startRecording();
              } catch (error) {
                startError = error;
              }

              if (startError === undefined) {
                throw new Error(
                  'Recording unexpectedly started under playback-only session settings'
                );
              }

              info(
                'Recorder rejected the wrong category as expected',
                startError instanceof Error
                  ? startError.message
                  : String(startError)
              );

              resourcesRef.current.recorder?.clearOnAudioReady();
              await AudioManager.setAudioSessionActivity(false);
            }
          );
          await run(
            'Switch back to playAndRecord and confirm clean recording works',
            performCleanRecording
          );
        }
      );

      await runScenario('Final Clean Cycle', async ({ run }) => {
        await run('Run one final clean record and playback cycle', async () => {
          const capture = await performCleanRecording();
          await performCleanPlayback(
            capture.decodedBuffer,
            1.4,
            'Final Clean Cycle: playback after recording'
          );
        });
      });

      appendLog('Suite completed.');
      showStep('Suite completed.');
    } catch (error) {
      if (error instanceof StopRequestedError) {
        appendLog('Suite stopped before completion.');
        showStep('Suite stopped.');
      } else {
        const serializedError = serializeUnknownError(error, 'Suite');
        appendLog(`Suite aborted: ${serializedError.headline}`);
        showStep(serializedError.headline);
      }
    } finally {
      await resourcesRef.current.cleanup();
      setRunnerState('finished');
    }
  };

  const resetSuite = async () => {
    if (isBusy) {
      return;
    }

    showStep('Resetting resources...');
    setScenarioResults([]);
    clearLog();
    await resourcesRef.current.cleanup();
    showStep(READY_MESSAGE);
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

  const countWithStatus = (status: ScenarioStatus) =>
    String(scenarioResults.filter((result) => result.status === status).length);
  const controlActions: ControlAction[] = [
    { title: 'Run Suite', onPress: runSuite, disabled: isBusy, minWidth: 120 },
    {
      title: 'Stop',
      onPress: requestStop,
      disabled: runnerState !== 'running',
      minWidth: 90,
    },
    { title: 'Reset', onPress: resetSuite, disabled: isBusy, minWidth: 90 },
  ];

  return (
    <TestUI.TestLayout
      description="Automated iOS playback and recording pipeline stress suite."
      summary={[
        { label: 'State', value: runnerState },
        { label: 'Passed', value: countWithStatus('pass') },
        { label: 'Failed', value: countWithStatus('fail') },
        { label: 'Skipped', value: countWithStatus('skipped') },
      ]}
      currentStep={currentStep}
      actions={controlActions}
    >
      <TestUI.ScenarioResults
        scenarios={scenarioResults}
        emptyMessage="No scenario results yet."
      />
      <TestUI.LiveLog
        entries={liveLog}
        emptyMessage="Live log will appear here while the suite runs."
      />
    </TestUI.TestLayout>
  );
};

export default AudioPipelineStress;
