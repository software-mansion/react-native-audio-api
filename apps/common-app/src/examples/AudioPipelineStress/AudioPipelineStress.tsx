import React, { FC, useEffect, useRef, useState } from 'react';
import { Platform, ScrollView, StyleSheet, Text, View } from 'react-native';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
  AudioManager,
  AudioRecorder,
  FileDirectory,
  FileFormat,
} from 'react-native-audio-api';

import { Button, Container, Spacer } from '../../components';
import { colors, layout } from '../../styles';
import staticAsset from '../AudioFile/voice-sample-landing.mp3';

type RunnerState = 'idle' | 'running' | 'stopping' | 'finished';

type ScenarioId =
  | 'playback_warmup'
  | 'record_warmup'
  | 'record_to_playback_loop'
  | 'playback_to_record_loop'
  | 'playback_session_deactivation_mid_run'
  | 'record_session_deactivation_mid_run'
  | 'wrong_category_then_recover'
  | 'final_clean_cycle';

type StepStatus = 'pass' | 'fail' | 'info' | 'skipped';
type ScenarioStatus = 'pass' | 'fail' | 'skipped';

interface StepResult {
  id: string;
  message: string;
  status: StepStatus;
  startedAt: number;
  finishedAt: number;
  details?: string;
}

interface ScenarioResult {
  scenarioId: ScenarioId;
  label: string;
  status: ScenarioStatus;
  startedAt: number;
  finishedAt: number;
  steps: StepResult[];
  error?: string;
}

interface RecordingCapture {
  decodedBuffer: AudioBuffer;
  fileDurationSeconds: number;
  path: string;
}

interface ReadyResources {
  assetBuffer: AudioBuffer;
  context: AudioContext;
  playback: StressPlaybackController;
  recorder: AudioRecorder;
}

interface RecorderHostShape {
  isPaused?: unknown;
}

interface SerializedError {
  headline: string;
  details: string;
}

interface PlaybackProgressStats {
  engineEndTimeSeconds: number;
  engineDeltaSeconds: number;
  engineStartTimeSeconds: number;
  engineSamples: number[];
  positionMaxObservedSeconds: number;
  positionSamples: number[];
  thresholdSeconds: number;
}

const DEFAULT_LOOP_COUNT = 5;
const PLAYBACK_PROGRESS_TIMEOUT_MS = 4000;
const RECORDING_CALLBACK_TIMEOUT_MS = 4000;
const POSITION_POLL_INTERVAL_MS = 150;
const PLAYBACK_STALL_WINDOW_MS = 900;
const RECORDING_STALL_WINDOW_MS = 900;
const SHORT_RECORDING_MS = 900;
const SHORT_PLAYBACK_MS = 1100;
const BUFFER_LENGTH = 4096;
const MIN_DECODED_DURATION_SECONDS = 0.2;

class StopRequestedError extends Error {
  constructor() {
    super('Run stopped by user');
    this.name = 'StopRequestedError';
  }
}

class StressPlaybackController {
  private readonly context: AudioContext;
  private source: AudioBufferSourceNode | null = null;
  private lastPositionSeconds = 0;
  private ended = false;

  constructor(context: AudioContext) {
    this.context = context;
  }

  async play(buffer: AudioBuffer, durationSeconds?: number): Promise<void> {
    this.stop();

    if (this.context.state === 'suspended') {
      await this.context.resume();
    }

    const source = this.context.createBufferSource({
      pitchCorrection: true,
    });

    this.lastPositionSeconds = 0;
    this.ended = false;
    this.source = source;
    source.buffer = buffer;
    source.onPositionChangedInterval = 50;
    source.onPositionChanged = (event) => {
      this.lastPositionSeconds = event.value;
    };
    source.onEnded = () => {
      this.ended = true;
    };
    source.connect(this.context.destination);
    source.start(this.context.currentTime, 0, durationSeconds);
  }

  stop(): void {
    if (!this.source) {
      return;
    }

    this.source.onEnded = null;
    this.source.onPositionChanged = null;

    try {
      this.source.stop(this.context.currentTime);
    } catch {
      // Source nodes cannot always be stopped twice safely.
    }

    this.source = null;

    this.context.suspend();
  }

  snapshot() {
    return {
      ended: this.ended,
      lastPositionSeconds: this.lastPositionSeconds,
      isActive: this.source !== null,
    };
  }
}

class StressResourceOwner {
  public context: AudioContext | null = null;
  public recorder: AudioRecorder | null = null;
  public playback: StressPlaybackController | null = null;
  public assetBuffer: AudioBuffer | null = null;
  public lastRecorderError: string | null = null;
  public callbackCount = 0;
  public lastCallbackFrames = 0;

  private readonly asset: string | number;

  constructor(asset: string | number) {
    this.asset = asset;
  }

  async recreate(): Promise<void> {
    await this.cleanup();

    const context = new AudioContext();
    const recorder = new AudioRecorder();
    const recorderHost = (
      recorder as unknown as { recorder?: RecorderHostShape }
    ).recorder;

    if (typeof recorderHost?.isPaused !== 'function') {
      throw new Error(
        `Recorder host object is missing isPaused(); typeof isPaused=${typeof recorderHost?.isPaused}`
      );
    }

    const fileOutputResult = recorder.enableFileOutput({
      channelCount: 1,
      directory: FileDirectory.Cache,
      fileNamePrefix: 'audio-pipeline-stress',
      format: FileFormat.M4A,
      subDirectory: 'AudioPipelineStress',
    });

    if (fileOutputResult.status === 'error') {
      throw new Error(
        `Failed to enable recorder file output: ${fileOutputResult.message}`
      );
    }

    recorder.onError((event) => {
      this.lastRecorderError = event.message;
    });

    this.context = context;
    this.recorder = recorder;
    this.playback = new StressPlaybackController(context);
    this.assetBuffer = await context.decodeAudioData(this.asset);
    this.lastRecorderError = null;
    this.callbackCount = 0;
    this.lastCallbackFrames = 0;
  }

  getReadyResources(): ReadyResources {
    if (
      !this.context ||
      !this.recorder ||
      !this.playback ||
      !this.assetBuffer
    ) {
      throw new Error('Audio pipeline resources are not ready');
    }

    return {
      assetBuffer: this.assetBuffer,
      context: this.context,
      playback: this.playback,
      recorder: this.recorder,
    };
  }

  configureRecorderTap(): void {
    const { context, recorder } = this.getReadyResources();
    this.callbackCount = 0;
    this.lastCallbackFrames = 0;
    this.lastRecorderError = null;

    const callbackResult = recorder.onAudioReady(
      {
        sampleRate: context.sampleRate,
        channelCount: 1,
        bufferLength: BUFFER_LENGTH,
      },
      (event) => {
        this.callbackCount += 1;
        this.lastCallbackFrames = event.numFrames;
      }
    );

    if (callbackResult.status === 'error') {
      throw new Error(
        `Failed to attach recorder callback: ${callbackResult.message}`
      );
    }
  }

  startRecording(fileNameOverride: string): void {
    const { recorder } = this.getReadyResources();
    const result = recorder.start({ fileNameOverride });

    if (result.status === 'error') {
      throw new Error(`Failed to start recording: ${result.message}`);
    }
  }

  tryStartRecording(fileNameOverride: string) {
    const { recorder } = this.getReadyResources();
    return recorder.start({ fileNameOverride });
  }

  async stopRecordingAndDecode(): Promise<RecordingCapture> {
    const { context, recorder } = this.getReadyResources();

    const stopResult = recorder.stop();
    recorder.clearOnAudioReady();

    if (stopResult.status === 'error') {
      throw new Error(`Failed to stop recording: ${stopResult.message}`);
    }

    if (!stopResult.path) {
      throw new Error('Recorder stop returned an empty file path');
    }

    const decodedBuffer = await context.decodeAudioData(stopResult.path);

    if (decodedBuffer.duration < MIN_DECODED_DURATION_SECONDS) {
      throw new Error(
        `Decoded buffer is too short: ${decodedBuffer.duration.toFixed(3)}s`
      );
    }

    return {
      decodedBuffer,
      fileDurationSeconds: stopResult.duration,
      path: stopResult.path,
    };
  }

  async cleanup(): Promise<void> {
    const recorder = this.recorder;
    const playback = this.playback;
    const context = this.context;

    try {
      playback?.stop();
    } catch {}

    try {
      recorder?.clearOnAudioReady();
    } catch {}

    try {
      recorder?.clearOnError();
    } catch {}

    try {
      if (recorder && (recorder.isRecording() || recorder.isPaused())) {
        recorder.stop();
      }
    } catch {}

    try {
      recorder?.disableFileOutput();
    } catch {}

    try {
      if (context?.state === 'running') {
        await context.suspend();
      }
    } catch {}

    try {
      await context?.close();
    } catch {}

    try {
      await AudioManager.setAudioSessionActivity(false);
    } catch {}

    this.context = null;
    this.recorder = null;
    this.playback = null;
    this.assetBuffer = null;
    this.lastRecorderError = null;
    this.callbackCount = 0;
    this.lastCallbackFrames = 0;
  }
}

async function sleep(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function formatDurationMs(durationMs: number): string {
  return `${(durationMs / 1000).toFixed(2)}s`;
}

function formatTimestamp(timestamp: number): string {
  const date = new Date(timestamp);
  return date.toLocaleTimeString([], {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  });
}

function getPlaybackProgressThreshold(durationSeconds: number): number {
  return Number(
    Math.max(0.08, Math.min(durationSeconds * 0.15, 0.18)).toFixed(2)
  );
}

function serializeUnknownError(
  error: unknown,
  contextLabel?: string
): SerializedError {
  const prefix = contextLabel ? `${contextLabel}: ` : '';

  if (error instanceof Error) {
    const headline = `${prefix}${error.name}: ${error.message}`;
    const details = [headline, error.stack].filter(Boolean).join('\n');

    return { headline, details };
  }

  const type = typeof error;
  const constructorName =
    error && typeof error === 'object' && 'constructor' in error
      ? ((error as { constructor?: { name?: string } }).constructor?.name ??
        'unknown')
      : 'n/a';
  const keys =
    error && typeof error === 'object' ? Object.keys(error as object) : [];

  let renderedValue = '';

  try {
    renderedValue =
      typeof error === 'string' ? error : JSON.stringify(error, null, 2);
  } catch {
    renderedValue = String(error);
  }

  const headline = `${prefix}Non-Error throw: ${String(error)}`;
  const details = [
    headline,
    `typeof=${type}`,
    `constructor=${constructorName}`,
    `keys=${keys.join(', ') || 'none'}`,
    `value=${renderedValue}`,
  ].join('\n');

  return { headline, details };
}

function formatPlaybackProgressStats(stats: PlaybackProgressStats): string {
  const diagnosis =
    stats.engineDeltaSeconds < 0.05
      ? 'engine-clock-stalled'
      : stats.positionMaxObservedSeconds < stats.thresholdSeconds
        ? 'engine-running-node-stalled'
        : 'engine-and-node-running';

  return [
    `diagnosis=${diagnosis}`,
    `threshold=${stats.thresholdSeconds.toFixed(2)}s`,
    `positionMax=${stats.positionMaxObservedSeconds.toFixed(3)}s`,
    `engineDelta=${stats.engineDeltaSeconds.toFixed(3)}s`,
    `engineStart=${stats.engineStartTimeSeconds.toFixed(3)}s`,
    `engineEnd=${stats.engineEndTimeSeconds.toFixed(3)}s`,
    `recentPositionSamples=[${stats.positionSamples
      .slice(-8)
      .map((value) => value.toFixed(3))
      .join(', ')}]`,
    `recentEngineSamples=[${stats.engineSamples
      .slice(-8)
      .map((value) => value.toFixed(3))
      .join(', ')}]`,
  ].join(', ');
}

async function waitForCondition(
  condition: () => boolean,
  timeoutMs: number,
  failureMessage: string,
  intervalMs: number = POSITION_POLL_INTERVAL_MS
): Promise<void> {
  const startedAt = Date.now();

  while (Date.now() - startedAt < timeoutMs) {
    if (condition()) {
      return;
    }

    await sleep(intervalMs);
  }

  throw new Error(failureMessage);
}

async function activatePlaybackSession(): Promise<void> {
  AudioManager.setAudioSessionOptions({
    iosCategory: 'playback',
    iosMode: 'default',
    iosOptions: [],
  });

  const success = await AudioManager.setAudioSessionActivity(true);

  if (!success) {
    throw new Error('Failed to activate playback session');
  }
}

async function activateRecordingSession(): Promise<void> {
  AudioManager.setAudioSessionOptions({
    iosCategory: 'playAndRecord',
    iosMode: 'default',
    iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
  });

  const success = await AudioManager.setAudioSessionActivity(true);

  if (!success) {
    throw new Error('Failed to activate recording session');
  }
}

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
    return () => {
      isMountedRef.current = false;
      stopRequestedRef.current = true;
      resourcesRef.current.cleanup().catch((error) => {
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
    await sleep(1500);
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
    await sleep(1500);
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
    await sleep(1500);
    await playback.play(buffer, durationSeconds);
    const playbackStats = await waitForPlaybackProgress(
      progressThreshold,
      contextLabel
    );
    await sleep(SHORT_PLAYBACK_MS);
    playback.stop();
    await AudioManager.setAudioSessionActivity(false);
    await sleep(1500);
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

                await sleep(1500);

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
        <Text style={styles.unsupportedTitle}>Audio Pipeline Stress</Text>
        <Spacer.Vertical size={12} />
        <Text style={styles.unsupportedText}>
          This screen is iOS-only because it validates the shared iOS audio
          session and engine pipeline.
        </Text>
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

  return (
    <Container disablePadding>
      <View style={styles.header}>
        <Text style={styles.title}>Audio Pipeline Stress</Text>
        <Text style={styles.subtitle}>
          Automated iOS playback and recording pipeline stress suite.
        </Text>
      </View>

      <View style={styles.summaryRow}>
        <SummaryBadge label="State" value={runnerState} />
        <SummaryBadge label="Passed" value={String(passedScenarios)} />
        <SummaryBadge label="Failed" value={String(failedScenarios)} />
        <SummaryBadge label="Skipped" value={String(skippedScenarios)} />
      </View>

      <View style={styles.stepCard}>
        <Text style={styles.stepLabel}>Current step</Text>
        <Text style={styles.stepText}>{currentStep}</Text>
      </View>

      <View style={styles.controls}>
        <Button
          title="Run Suite"
          onPress={() => {
            runSuite();
          }}
          disabled={runnerState === 'running' || runnerState === 'stopping'}
          width={120}
        />
        <Button
          title="Stop"
          onPress={requestStop}
          disabled={runnerState !== 'running'}
          width={90}
        />
        <Button
          title="Reset"
          onPress={() => {
            resetSuite();
          }}
          disabled={runnerState === 'running' || runnerState === 'stopping'}
          width={90}
        />
      </View>

      <ScrollView
        style={styles.scrollView}
        contentContainerStyle={styles.scrollContent}
      >
        <SectionTitle title="Scenario Results" />
        {scenarioResults.length === 0 ? (
          <EmptyState message="No scenario results yet." />
        ) : (
          scenarioResults.map((scenario) => (
            <View
              key={`${scenario.scenarioId}-${scenario.startedAt}`}
              style={styles.scenarioCard}
            >
              <View style={styles.scenarioHeader}>
                <Text style={styles.scenarioTitle}>{scenario.label}</Text>
                <StatusPill status={scenario.status} />
              </View>
              <Text style={styles.scenarioMeta}>
                {formatDurationMs(scenario.finishedAt - scenario.startedAt)}
              </Text>
              {scenario.steps.map((step) => (
                <View
                  key={`${scenario.scenarioId}-${step.id}`}
                  style={styles.stepRow}
                >
                  <View style={styles.stepRowHeader}>
                    <Text style={stepStatusStyle(step.status)}>
                      {step.status.toUpperCase()}
                    </Text>
                    <Text style={styles.stepMessage}>{step.message}</Text>
                  </View>
                  {step.details ? (
                    <Text style={styles.stepDetails}>{step.details}</Text>
                  ) : null}
                </View>
              ))}
            </View>
          ))
        )}

        <SectionTitle title="Live Log" />
        {liveLog.length === 0 ? (
          <EmptyState message="Live log will appear here while the suite runs." />
        ) : (
          <View style={styles.logCard}>
            {liveLog.map((entry, index) => (
              <Text key={`${entry}-${index}`} style={styles.logLine}>
                {entry}
              </Text>
            ))}
          </View>
        )}
      </ScrollView>
    </Container>
  );
};

const SectionTitle: FC<{ title: string }> = ({ title }) => (
  <Text style={styles.sectionTitle}>{title}</Text>
);

const EmptyState: FC<{ message: string }> = ({ message }) => (
  <View style={styles.emptyCard}>
    <Text style={styles.emptyText}>{message}</Text>
  </View>
);

const SummaryBadge: FC<{ label: string; value: string }> = ({
  label,
  value,
}) => (
  <View style={styles.summaryBadge}>
    <Text style={styles.summaryLabel}>{label}</Text>
    <Text style={styles.summaryValue}>{value}</Text>
  </View>
);

const StatusPill: FC<{ status: ScenarioStatus }> = ({ status }) => (
  <View
    style={[
      styles.statusPill,
      status === 'pass'
        ? styles.statusPass
        : status === 'fail'
          ? styles.statusFail
          : styles.statusSkipped,
    ]}
  >
    <Text style={styles.statusText}>{status.toUpperCase()}</Text>
  </View>
);

const stepStatusStyle = (status: StepStatus) => ({
  color:
    status === 'pass'
      ? '#7bd88f'
      : status === 'fail'
        ? '#ff8d8d'
        : status === 'skipped'
          ? colors.gray
          : '#8fc8ff',
  fontSize: 11,
  fontWeight: '700' as const,
  minWidth: 44,
});

const styles = StyleSheet.create({
  controls: {
    flexDirection: 'row',
    gap: layout.spacing,
    paddingHorizontal: 18,
    paddingBottom: 12,
  },
  emptyCard: {
    backgroundColor: colors.backgroundLight,
    borderRadius: layout.radius,
    padding: 16,
  },
  emptyText: {
    color: colors.gray,
  },
  header: {
    gap: 6,
    paddingHorizontal: 18,
    paddingTop: 18,
    paddingBottom: 12,
  },
  logCard: {
    backgroundColor: colors.backgroundLight,
    borderRadius: layout.radius,
    gap: 8,
    padding: 16,
  },
  logLine: {
    color: colors.gray,
    fontSize: 12,
    lineHeight: 18,
  },
  scenarioCard: {
    backgroundColor: colors.backgroundLight,
    borderRadius: layout.radius,
    gap: 10,
    padding: 16,
  },
  scenarioHeader: {
    alignItems: 'center',
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  scenarioMeta: {
    color: colors.gray,
    fontSize: 12,
  },
  scenarioTitle: {
    color: colors.white,
    flex: 1,
    fontSize: 16,
    fontWeight: '700',
    paddingRight: 12,
  },
  scrollContent: {
    gap: 12,
    paddingHorizontal: 18,
    paddingTop: 18,
    paddingBottom: 32,
  },
  scrollView: {
    flex: 1,
  },
  sectionTitle: {
    color: colors.white,
    fontSize: 18,
    fontWeight: '700',
  },
  statusFail: {
    backgroundColor: '#a63f3f',
  },
  statusPass: {
    backgroundColor: '#28794a',
  },
  statusPill: {
    borderRadius: 999,
    paddingHorizontal: 10,
    paddingVertical: 5,
  },
  statusSkipped: {
    backgroundColor: '#5b5f6c',
  },
  statusText: {
    color: colors.white,
    fontSize: 11,
    fontWeight: '700',
  },
  stepCard: {
    backgroundColor: colors.backgroundLight,
    borderRadius: layout.radius,
    gap: 8,
    marginHorizontal: 18,
    marginBottom: 16,
    marginTop: 18,
    padding: 16,
  },
  stepDetails: {
    color: colors.gray,
    fontSize: 12,
    lineHeight: 17,
    paddingLeft: 56,
  },
  stepLabel: {
    color: colors.gray,
    fontSize: 12,
    textTransform: 'uppercase',
  },
  stepMessage: {
    color: colors.white,
    flex: 1,
    fontSize: 13,
  },
  stepRow: {
    gap: 6,
  },
  stepRowHeader: {
    alignItems: 'center',
    flexDirection: 'row',
    gap: 12,
  },
  stepText: {
    color: colors.white,
    fontSize: 14,
    lineHeight: 20,
  },
  subtitle: {
    color: colors.gray,
    fontSize: 14,
    lineHeight: 20,
  },
  summaryBadge: {
    backgroundColor: colors.backgroundLight,
    borderRadius: layout.radius,
    flex: 1,
    gap: 4,
    padding: 12,
  },
  summaryLabel: {
    color: colors.gray,
    fontSize: 11,
    textTransform: 'uppercase',
  },
  summaryRow: {
    flexDirection: 'row',
    gap: 8,
    paddingHorizontal: 18,
  },
  summaryValue: {
    color: colors.white,
    fontSize: 16,
    fontWeight: '700',
  },
  title: {
    color: colors.white,
    fontSize: 24,
    fontWeight: '700',
  },
  unsupportedText: {
    color: colors.gray,
    fontSize: 14,
    lineHeight: 20,
    maxWidth: 280,
    textAlign: 'center',
  },
  unsupportedTitle: {
    color: colors.white,
    fontSize: 24,
    fontWeight: '700',
  },
});

export default AudioPipelineStress;
