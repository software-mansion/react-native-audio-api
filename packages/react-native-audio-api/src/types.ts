import AudioBuffer from './core/AudioBuffer';
import PeriodicWave from './core/PeriodicWave';
import { IAudioBuffer } from './interfaces';

export type Result<T> =
  | ({ status: 'success' } & T)
  | { status: 'error'; message: string };

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

export type ContextState = 'running' | 'closed' | 'suspended';

export type AudioWorkletRuntime = 'AudioRuntime' | 'UIRuntime';

export type OscillatorType =
  | 'sine'
  | 'square'
  | 'sawtooth'
  | 'triangle'
  | 'custom';

export interface AudioContextOptions {
  sampleRate?: number;
}

export interface OfflineAudioContextOptions {
  numberOfChannels: number;
  length: number;
  sampleRate: number;
}

export enum FileDirectory {
  Document = 0,
  Cache = 1,
}

export enum FileFormat {
  Wav = 0,
  Caf = 1,
  M4A = 2,
  Flac = 3,
}

export enum IOSAudioQuality {
  Min = 0,
  Low = 1,
  Medium = 2,
  High = 3,
  Max = 4,
}

export enum BitDepth {
  Bit16 = 0,
  Bit24 = 1,
  Bit32 = 2,
}

export enum FlacCompressionLevel {
  L0 = 0,
  L1 = 1,
  L2 = 2,
  L3 = 3,
  L4 = 4,
  L5 = 5,
  L6 = 6,
  L7 = 7,
  L8 = 8,
}

export interface FilePresetType {
  bitRate: number;
  sampleRate: number;
  bitDepth: BitDepth;
  iosQuality: IOSAudioQuality;
  flacCompressionLevel: FlacCompressionLevel;
}

export interface AudioRecorderFileOptions {
  channelCount?: number;
  batchDurationSeconds?: number;

  format?: FileFormat;
  preset?: FilePresetType;

  directory?: FileDirectory;
  subDirectory?: string;
  fileNamePrefix?: string;
  androidFlushIntervalMs?: number;
}

export interface FileInfo {
  path: string;
  size: number;
  duration: number;
}

export type WindowType = 'blackman' | 'hann';

export type ProcessorMode = 'processInPlace' | 'processThrough';

export interface AudioNodeOptions {
  channelCount?: number;
  channelCountMode?: ChannelCountMode;
  channelInterpretation?: ChannelInterpretation;
}

export interface GainOptions extends AudioNodeOptions {
  gain?: number;
}

export interface StereoPannerOptions extends AudioNodeOptions {
  pan?: number;
}

export interface AnalyserOptions extends AudioNodeOptions {
  fftSize?: number;
  minDecibels?: number;
  maxDecibels?: number;
  smoothingTimeConstant?: number;
}

export interface OptionsValidator<T> {
  validate(options?: T): void;
}

export interface BiquadFilterOptions extends AudioNodeOptions {
  type?: BiquadFilterType;
  frequency?: number;
  detune?: number;
  Q?: number;
  gain?: number;
}

export interface OscillatorOptions {
  type?: OscillatorType;
  frequency?: number;
  detune?: number;
  periodicWave?: PeriodicWave;
}

export interface BaseAudioBufferSourceOptions {
  detune?: number;
  playbackRate?: number;
  pitchCorrection?: boolean;
}

export interface AudioBufferSourceOptions extends BaseAudioBufferSourceOptions {
  buffer?: AudioBuffer;
  loop?: boolean;
  loopStart?: number;
  loopEnd?: number;
}

// options that are passed to c++ layer
export interface IAudioBufferSourceOptions
  extends BaseAudioBufferSourceOptions {
  buffer?: IAudioBuffer;
  loop?: boolean;
  loopStart?: number;
  loopEnd?: number;
}

export interface ConvolverOptions extends AudioNodeOptions {
  buffer?: AudioBuffer;
  disableNormalization?: boolean;
}

// options that are passed to c++ layer
export interface IConvolverOptions extends AudioNodeOptions {
  buffer?: IAudioBuffer;
  disableNormalization?: boolean;
}

export interface ConstantSourceOptions {
  offset?: number;
}

export interface StreamerOptions {
  streamPath?: string;
}

export interface PeriodicWaveConstraints {
  disableNormalization?: boolean;
}

export interface PeriodicWaveOptions extends PeriodicWaveConstraints {
  real?: Float32Array;
  imag?: Float32Array;
}

export interface AudioBufferOptions {
  length: number;
  numberOfChannels?: number;
  sampleRate: number;
}

export interface IIRFilterOptions extends AudioNodeOptions {
  feedforward: number[];
  feedback: number[];
}
export type OverSampleType = 'none' | '2x' | '4x';

export interface AudioRecorderCallbackOptions {
  /**
   * The desired sample rate (in Hz) for audio buffers delivered to the
   * recording callback. Common values include 44100 or 48000 Hz. The actual
   * sample rate may differ depending on hardware and system capabilities.
   */
  sampleRate: number;

  /**
   * The preferred size of each audio buffer, expressed as the number of samples
   * per channel. Smaller buffers reduce latency but increase CPU load, while
   * larger buffers improve efficiency at the cost of higher latency.
   */
  bufferLength: number;

  /**
   * The desired number of audio channels per buffer. Typically 1 for mono or 2
   * for stereo recordings.
   */
  channelCount: number;
}

export interface DelayOptions extends AudioNodeOptions {
  maxDelayTime?: number;
  delayTime?: number;
}

export interface WaveShaperOptions extends AudioNodeOptions {
  curve?: Float32Array;
  oversample?: OverSampleType;
}

export type DecodeDataInput = number | string | ArrayBuffer;

export interface AudioRecorderStartOptions {
  fileNameOverride?: string;
}
