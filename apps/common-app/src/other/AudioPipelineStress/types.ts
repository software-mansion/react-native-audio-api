import type {
  AudioBuffer,
  AudioContext,
  AudioRecorder,
} from 'react-native-audio-api';

import type StressPlaybackController from './StressPlaybackController';

export type RunnerState = 'idle' | 'running' | 'stopping' | 'finished';

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
