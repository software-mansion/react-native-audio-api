import type { AudioBuffer, AudioContext, AudioRecorder } from 'react-native-audio-api';

import type StressPlaybackController from './StressPlaybackController';

export type RunnerState = 'idle' | 'running' | 'stopping' | 'finished';

export type ScenarioId =
  | 'playback_warmup'
  | 'record_warmup'
  | 'record_to_playback_loop'
  | 'playback_to_record_loop'
  | 'playback_session_deactivation_mid_run'
  | 'record_session_deactivation_mid_run'
  | 'wrong_category_then_recover'
  | 'final_clean_cycle';

export type StepStatus = 'pass' | 'fail' | 'info' | 'skipped';
export type ScenarioStatus = 'pass' | 'fail' | 'skipped';

export interface StepResult {
  id: string;
  message: string;
  status: StepStatus;
  startedAt: number;
  finishedAt: number;
  details?: string;
}

export interface ScenarioResult {
  scenarioId: ScenarioId;
  label: string;
  status: ScenarioStatus;
  startedAt: number;
  finishedAt: number;
  steps: StepResult[];
  error?: string;
}

export interface RecordingCapture {
  decodedBuffer: AudioBuffer;
  fileDurationSeconds: number;
  path: string;
}

export interface ReadyResources {
  assetBuffer: AudioBuffer;
  context: AudioContext;
  playback: StressPlaybackController;
  recorder: AudioRecorder;
}

export interface SerializedError {
  headline: string;
  details: string;
}

export interface PlaybackProgressStats {
  engineEndTimeSeconds: number;
  engineDeltaSeconds: number;
  engineStartTimeSeconds: number;
  engineSamples: number[];
  positionMaxObservedSeconds: number;
  positionSamples: number[];
  thresholdSeconds: number;
}
