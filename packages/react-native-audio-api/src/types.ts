import type PeriodicWave from './core/PeriodicWave';

export interface AudioBufferLike {
  readonly length: number;
  readonly duration: number;
  readonly sampleRate: number;
  readonly numberOfChannels: number;
  getChannelData(channel: number): Float32Array<ArrayBuffer>;
  copyFromChannel(
    dest: Float32Array<ArrayBuffer>,
    channelNumber: number,
    startInChannel?: number
  ): void;
  copyToChannel(
    source: Float32Array<ArrayBuffer>,
    channelNumber: number,
    startInChannel?: number
  ): void;
}

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

export type OscillatorType =
  | 'sine'
  | 'square'
  | 'sawtooth'
  | 'triangle'
  | 'custom';

export type AudioContextLatencyCategory =
  | 'balanced'
  | 'interactive'
  | 'playback';

export interface AudioContextOptions {
  sampleRate?: number;
  /**
   * What the context should optimize its output stream for; defaults to
   * `'interactive'`. See the platform table in the docs. Numeric hints are not
   * supported yet.
   */
  latencyHint?: AudioContextLatencyCategory;
  androidOutputProfile?: AndroidOutputProfile;
}

export type AndroidOutputProfile = 'media' | 'voiceCommunication';

export interface OfflineAudioContextOptions {
  numberOfChannels: number;
  length: number;
  sampleRate: number;
}

export enum FileDirectory {
  Document = 0,
  Cache = 1,
}

// Values must stay in sync with the C++ `AudioFileProperties::Format` enum.
// Support is platform-dependent; unsupported formats fail when recording starts.
// See the audio recorder docs for the per-platform format matrix.
export enum FileFormat {
  Wav,
  Caf,
  M4A,
  Adts,
  Flac,
  Aiff,
  Alac,
  OpusOgg,
  OpusWebm,
  VorbisWebm,
  Ulaw,
  Alaw,
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

export type AndroidInputPreset =
  | 'generic'
  | 'camcorder'
  | 'voiceRecognition'
  | 'voiceCommunication'
  | 'unprocessed'
  | 'voicePerformance';

/** Configures the platform capture chain when the recorder is created. */
export interface AudioRecorderOptions {
  /**
   * Preprocessing chain applied to the capture stream (Android only), mapped to
   * Oboe's
   * [`InputPreset`](https://github.com/google/oboe/blob/0da326e4ef878eac0c032e11ea84ca0a6811aafd/include/oboe/Definitions.h#L470).
   * The platform default, `voiceRecognition`, applies no acoustic echo
   * cancellation - use `voiceCommunication` to engage the platform AEC/NS
   * chain.
   */
  androidInputPreset?: AndroidInputPreset;

  /**
   * Runs the capture chain through Apple's voice-processing I/O (iOS only):
   * acoustic echo cancellation, noise suppression and automatic gain control.
   * Defaults to `false`, which keeps the raw microphone signal.
   */
  iosVoiceProcessing?: boolean;
}

/**
 * Options of {@link encodeAudioData}. The recorder's
 * {@link AudioRecorderFileOptions} extends them with its recording-only
 * fields.
 */
export interface AudioEncoderOptions {
  /** Defaults to {@link FileFormat.M4A}. */
  format?: FileFormat;
  /**
   * Sample rate (at most 48000 Hz) and encoding quality of the file. Defaults
   * to `FilePreset.High`; for `encodeAudioData`, with the first buffer's sample
   * rate, capped at 48000 Hz.
   */
  preset?: FilePresetType;
  /**
   * Of the file, 1 or 2. Defaults to 2 for the recorder and to the first
   * buffer's channel count, capped at 2, for `encodeAudioData`.
   */
  channelCount?: number;

  /** Defaults to {@link FileDirectory.Cache}. */
  directory?: FileDirectory;
  /** Defaults to `'AudioAPI'`. */
  subDirectory?: string;

  /**
   * Names the output file outright — no timestamp is added, so supply whatever
   * identity you need (a session id, your own timestamp). Left unset, the
   * library generates `recording_<timestamp>` for the recorder and
   * `encoded_<timestamp>` for `encodeAudioData`.
   *
   * Give a bare name: no extension (it follows from `format`), no path
   * separators and no `..`.
   *
   * An existing file of the same name is overwritten, with a warning in the
   * native log, so make the name unique yourself.
   */
  fileName?: string;
}

export interface AudioRecorderFileOptions extends AudioEncoderOptions {
  /**
   * Splits the recording into files of about this size. `_001`, `_002`, … are
   * then appended to the file name, because one recording spans several files.
   */
  rotateIntervalBytes?: number;

  androidFlushIntervalMs?: number;
}

/** What {@link FileInfo} and {@link EncodedFileInfo} share. */
export interface AudioFileStats {
  /** The size of the written file or files (in MB). */
  size: number;
  /** The duration of the audio (in seconds). */
  duration: number;
}

/** What `AudioRecorder.stop()` produced: one file, or several when rotating. */
export interface FileInfo extends AudioFileStats {
  paths: string[];
}

/** The one file {@link encodeAudioData} wrote. */
export interface EncodedFileInfo extends AudioFileStats {
  /** A `file://` URI. */
  path: string;
}

export type ProcessorMode = 'processInPlace' | 'processThrough';

export interface AudioNodeOptions {
  channelCount?: number;
  channelCountMode?: ChannelCountMode;
  channelInterpretation?: ChannelInterpretation;
}

export interface GainOptions extends AudioNodeOptions {
  gain?: number;
}

export interface ChannelMergerOptions extends AudioNodeOptions {
  numberOfInputs?: number;
}

export interface ChannelSplitterOptions extends AudioNodeOptions {
  numberOfOutputs?: number;
}

export interface StereoPannerOptions extends AudioNodeOptions {
  pan?: number;
}

export type PanningModelType = 'equalpower';

export type DistanceModelType = 'linear' | 'inverse' | 'exponential';

export interface PannerOptions extends AudioNodeOptions {
  panningModel?: PanningModelType;
  distanceModel?: DistanceModelType;
  positionX?: number;
  positionY?: number;
  positionZ?: number;
  orientationX?: number;
  orientationY?: number;
  orientationZ?: number;
  refDistance?: number;
  maxDistance?: number;
  rolloffFactor?: number;
  coneInnerAngle?: number;
  coneOuterAngle?: number;
  coneOuterGain?: number;
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

export interface OscillatorOptions extends AudioNodeOptions {
  type?: OscillatorType;
  frequency?: number;
  detune?: number;
  periodicWave?: PeriodicWave;
}

interface BaseAudioBufferSourceOptions extends AudioNodeOptions {
  detune?: number;
  playbackRate?: number;
  pitchCorrection?: boolean;
}

export type AudioBufferQueueSourceOptions = BaseAudioBufferSourceOptions;

export interface AudioBufferSourceOptions extends BaseAudioBufferSourceOptions {
  buffer?: AudioBufferLike;
  loop?: boolean;
  loopStart?: number;
  loopEnd?: number;
}

export interface ConvolverOptions extends AudioNodeOptions {
  buffer?: AudioBufferLike;
  disableNormalization?: boolean;
}

export interface AudioFileSourceOptions extends AudioNodeOptions {
  source: ArrayBuffer | string;
  headers?: Record<string, string>;
  loop?: boolean;
  volume?: number;
  playbackRate?: number;
  preservesPitch?: boolean;
}

export interface ConstantSourceOptions extends AudioNodeOptions {
  offset?: number;
}

export interface PeriodicWaveConstraints {
  disableNormalization?: boolean;
}

export interface PeriodicWaveOptions extends PeriodicWaveConstraints {
  real?: number[] | Float32Array;
  imag?: number[] | Float32Array;
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
  curve?: number[] | Float32Array;
  oversample?: OverSampleType;
}

export type DecodeDataInput = number | string | ArrayBuffer;
export type AudioDurationInput = string | ArrayBuffer;

export enum AutomationEventType {
  LINEAR_RAMP,
  EXPONENTIAL_RAMP,
  SET_VALUE,
  SET_TARGET,
  SET_VALUE_CURVE,
}

export type AutomationEventData =
  | { type: AutomationEventType.SET_VALUE; automationTime: number }
  | { type: AutomationEventType.LINEAR_RAMP; automationTime: number }
  | {
      type: AutomationEventType.EXPONENTIAL_RAMP;
      automationTime: number;
    }
  | {
      type: AutomationEventType.SET_TARGET;
      automationTime: number;
    }
  | {
      type: AutomationEventType.SET_VALUE_CURVE;
      automationTime: number;
      duration: number;
    };
