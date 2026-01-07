import {
  TAudioNodeOptions,
  TGainOptions,
  TSteroPannerOptions,
  TConvolverOptions,
  TConstantSourceOptions,
  TPeriodicWaveConstraints,
  TAnalyserOptions,
  TBiquadFilterOptions,
  TOscillatorOptions,
  TBaseAudioBufferSourceOptions,
  TAudioBufferSourceOptions,
  TAudioBufferOptions,
  TDelayOptions,
  TWaveShaperOptions,
} from './types';

export const AudioNodeOptions: TAudioNodeOptions = {
  channelCount: 2,
  channelCountMode: 'max',
  channelInterpretation: 'speakers',
};

export const GainOptions: TGainOptions = {
  ...AudioNodeOptions,
  gain: 1,
};

export const SteroPannerOptions: TSteroPannerOptions = {
  ...AudioNodeOptions,
  channelCountMode: 'clamped-max',
  pan: 0,
};

export const AnalyserOptions: TAnalyserOptions = {
  ...AudioNodeOptions,
  fftSize: 2048,
  minDecibels: -100,
  maxDecibels: -30,
  smoothingTimeConstant: 0.8,
};

export const BiquadFilterOptions: TBiquadFilterOptions = {
  ...AudioNodeOptions,
  Q: 1,
  detune: 0,
  frequency: 350,
  gain: 0,
  type: 'lowpass',
};

export const ConvolverOptions: TConvolverOptions = {
  ...AudioNodeOptions,
  disableNormalization: false,
};

export const ConstantSourceOptions: TConstantSourceOptions = {
  offset: 1,
};

export const PeriodicWaveConstraints: TPeriodicWaveConstraints = {
  disableNormalization: false,
};

export const OscillatorOptions: TOscillatorOptions = {
  ...AudioNodeOptions,
  type: 'sine',
  frequency: 440,
  detune: 0,
};

export const BaseAudioBufferSourceOptions: TBaseAudioBufferSourceOptions = {
  playbackRate: 1,
  detune: 0,
  pitchCorrection: false,
};

export const AudioBufferSourceOptions: TAudioBufferSourceOptions = {
  ...BaseAudioBufferSourceOptions,
  loop: false,
  loopStart: 0,
  loopEnd: 0,
};

export const AudioBufferOptions: TAudioBufferOptions = {
  numberOfChannels: 1,
  length: 0, // always overwritten by provided value, only placeholder
  sampleRate: 44100, // always overwritten by provided value, only placeholder
};

export const DelayOptions: TDelayOptions = {
  ...AudioNodeOptions,
  maxDelayTime: 1.0,
  delayTime: 0.0,
};

export const WaveShaperOptions: TWaveShaperOptions = {
  ...AudioNodeOptions,
  oversample: 'none',
};
