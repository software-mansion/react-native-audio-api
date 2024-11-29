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

export type ContextState = 'running' | 'closed';

export type OscillatorType =
  | 'sine'
  | 'square'
  | 'sawtooth'
  | 'triangle'
  | 'custom';

export interface PeriodicWaveConstraints {
  disableNormalization: boolean;
}

/**
 * A type that defines the source of the audio which can be expressed in several forms:
 * 1. A string path, which could be an HTTPS URL or a local file path.
 * 2. An object specifying more detailed information about the source, including URI and HTTP headers.
 */
export type AudioSource =
  | string
  | {
      uri?: string;
      headers?: Record<string, string>;
    };
