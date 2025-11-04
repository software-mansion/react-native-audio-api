import AudioBuffer from './core/AudioBuffer';

export type ChannelCountMode = 'max' | 'clamped-max' | 'explicit';

export type ChannelInterpretation = 'speakers' | 'discrete';

export type BiquadFilterType =
  | 'lowpass'
  | 'highpass'
  | 'bandpass'
  | 'lowshelf'
  | 'highshelf'
  | 'peaking'
  | 'notch'
  | 'allpass';

export type ContextState = 'running' | 'closed' | `suspended`;

export type AudioWorkletRuntime = 'AudioRuntime' | 'UIRuntime';

export type OscillatorType =
  | 'sine'
  | 'square'
  | 'sawtooth'
  | 'triangle'
  | 'custom';

export interface PeriodicWaveConstraints {
  disableNormalization: boolean;
}

export interface AudioContextOptions {
  sampleRate?: number;
  initSuspended?: boolean;
}

export interface OfflineAudioContextOptions {
  numberOfChannels: number;
  length: number;
  sampleRate: number;
}

export interface AudioRecorderOptions {
  sampleRate: number;
  bufferLengthInSamples: number;
}

export type WindowType = 'blackman' | 'hann';

export interface AudioBufferBaseSourceNodeOptions {
  pitchCorrection: boolean;
}

export type ProcessorMode = 'processInPlace' | 'processThrough';

export interface ConvolverNodeOptions {
  buffer?: AudioBuffer | null;
  disableNormalization?: boolean;
}
