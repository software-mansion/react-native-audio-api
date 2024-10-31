export type ChannelCountMode = 'max' | 'clamped-max' | 'explicit';

export type ChannelInterpretation = 'speakers' | 'discrete';

export type FilterType =
  | 'lowpass'
  | 'highpass'
  | 'bandpass'
  | 'lowshelf'
  | 'highshelf'
  | 'peaking'
  | 'notch'
  | 'allpass';

export type ContextState = 'running' | 'closed';

export type WaveType = 'sine' | 'square' | 'sawtooth' | 'triangle';
