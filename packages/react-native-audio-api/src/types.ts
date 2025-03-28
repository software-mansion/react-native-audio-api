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
  sampleRate: number;
}

export interface OfflineAudioContextOptions {
  numberOfChannels: number;
  length: number;
  sampleRate: number;
}

export type WindowType = 'blackman' | 'hann';

export interface AudioBufferSourceNodeOptions {
  pitchCorrection: boolean;
}
