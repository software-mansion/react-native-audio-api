import { AudioEventCallback, AudioEventName } from './events/types';
import {
  BiquadFilterType,
  ChannelCountMode,
  ChannelInterpretation,
  ContextState,
  OscillatorType,
  WindowType,
} from './types';

// IMPORTANT: use only IClass, because it is a part of contract between cpp host object and js layer

export type WorkletNodeCallback = (
  audioData: Array<ArrayBuffer>,
  channelCount: number
) => void;

export type WorkletSourceNodeCallback = (
  audioData: Array<ArrayBuffer>,
  framesToProcess: number,
  currentTime: number,
  startOffset: number
) => void;

export type WorkletProcessingNodeCallback = (
  inputData: Array<ArrayBuffer>,
  outputData: Array<ArrayBuffer>,
  framesToProcess: number,
  currentTime: number
) => void;

export type ShareableWorkletCallback =
  | WorkletNodeCallback
  | WorkletSourceNodeCallback
  | WorkletProcessingNodeCallback;

export interface IBaseAudioContext {
  readonly destination: IAudioDestinationNode;
  readonly state: ContextState;
  readonly sampleRate: number;
  readonly currentTime: number;
  readonly decoder: IAudioDecoder;
  readonly stretcher: IAudioStretcher;

  createRecorderAdapter(): IRecorderAdapterNode;
  createWorkletSourceNode(
    shareableWorklet: ShareableWorkletCallback,
    shouldUseUiRuntime: boolean
  ): IWorkletSourceNode;
  createWorkletNode(
    shareableWorklet: ShareableWorkletCallback,
    shouldUseUiRuntime: boolean,
    bufferLength: number,
    inputChannelCount: number
  ): IWorkletNode;
  createWorkletProcessingNode(
    shareableWorklet: ShareableWorkletCallback,
    shouldUseUiRuntime: boolean
  ): IWorkletProcessingNode;
  createOscillator(): IOscillatorNode;
  createConstantSource(): IConstantSourceNode;
  createGain(): IGainNode;
  createStereoPanner(): IStereoPannerNode;
  createBiquadFilter: () => IBiquadFilterNode;
  createBufferSource: (pitchCorrection: boolean) => IAudioBufferSourceNode;
  createBufferQueueSource: (
    pitchCorrection: boolean
  ) => IAudioBufferQueueSourceNode;
  createBuffer: (
    channels: number,
    length: number,
    sampleRate: number
  ) => IAudioBuffer;
  createPeriodicWave: (
    real: Float32Array,
    imag: Float32Array,
    disableNormalization: boolean
  ) => IPeriodicWave;
  createAnalyser: () => IAnalyserNode;
  createConvolver: (
    buffer: IAudioBuffer | undefined,
    disableNormalization: boolean
  ) => IConvolverNode;
  createStreamer: () => IStreamerNode;
}

export interface IAudioContext extends IBaseAudioContext {
  close(): Promise<void>;
  resume(): Promise<boolean>;
  suspend(): Promise<boolean>;
}

export interface IOfflineAudioContext extends IBaseAudioContext {
  resume(): Promise<void>;
  suspend(suspendTime: number): Promise<void>;
  startRendering(): Promise<IAudioBuffer>;
}

export interface IAudioNode {
  readonly context: BaseAudioContext;
  readonly numberOfInputs: number;
  readonly numberOfOutputs: number;
  readonly channelCount: number;
  readonly channelCountMode: ChannelCountMode;
  readonly channelInterpretation: ChannelInterpretation;

  connect: (destination: IAudioNode | IAudioParam) => void;
  disconnect: (destination?: IAudioNode | IAudioParam) => void;
}

export interface IGainNode extends IAudioNode {
  readonly gain: IAudioParam;
}

export interface IStereoPannerNode extends IAudioNode {
  readonly pan: IAudioParam;
}

export interface IBiquadFilterNode extends IAudioNode {
  readonly frequency: AudioParam;
  readonly detune: AudioParam;
  readonly Q: AudioParam;
  readonly gain: AudioParam;
  type: BiquadFilterType;

  getFrequencyResponse(
    frequencyArray: Float32Array,
    magResponseOutput: Float32Array,
    phaseResponseOutput: Float32Array
  ): void;
}

export interface IAudioDestinationNode extends IAudioNode {}

export interface IAudioScheduledSourceNode extends IAudioNode {
  start(when: number): void;
  stop: (when: number) => void;

  // passing subscriptionId(uint_64 in cpp, string in js) to the cpp
  onEnded: string;
}

export interface IAudioBufferBaseSourceNode extends IAudioScheduledSourceNode {
  detune: IAudioParam;
  playbackRate: IAudioParam;

  // passing subscriptionId(uint_64 in cpp, string in js) to the cpp
  onPositionChanged: string;
  // set how often the onPositionChanged event is called
  onPositionChangedInterval: number;
}

export interface IOscillatorNode extends IAudioScheduledSourceNode {
  readonly frequency: IAudioParam;
  readonly detune: IAudioParam;
  type: OscillatorType;

  setPeriodicWave(periodicWave: IPeriodicWave): void;
}

export interface IStreamerNode extends IAudioNode {
  initialize(streamPath: string): boolean;
}

export interface IConstantSourceNode extends IAudioScheduledSourceNode {
  readonly offset: IAudioParam;
}

export interface IAudioBufferSourceNode extends IAudioBufferBaseSourceNode {
  buffer: IAudioBuffer | null;
  loop: boolean;
  loopSkip: boolean;
  loopStart: number;
  loopEnd: number;

  start: (when?: number, offset?: number, duration?: number) => void;
  setBuffer: (audioBuffer: IAudioBuffer | null) => void;

  // passing subscriptionId(uint_64 in cpp, string in js) to the cpp
  onLoopEnded: string;
}

export interface IAudioBufferQueueSourceNode
  extends IAudioBufferBaseSourceNode {
  dequeueBuffer: (bufferId: number) => void;
  clearBuffers: () => void;

  // returns bufferId
  enqueueBuffer: (audioBuffer: IAudioBuffer) => string;
  pause: () => void;
}

export interface IConvolverNode extends IAudioNode {
  buffer: IAudioBuffer | null;
  normalize: boolean;
}

export interface IAudioBuffer {
  readonly length: number;
  readonly duration: number;
  readonly sampleRate: number;
  readonly numberOfChannels: number;

  getChannelData(channel: number): Float32Array;
  copyFromChannel(
    destination: Float32Array,
    channelNumber: number,
    startInChannel: number
  ): void;
  copyToChannel(
    source: Float32Array,
    channelNumber: number,
    startInChannel: number
  ): void;
}

export interface IAudioParam {
  value: number;
  defaultValue: number;
  minValue: number;
  maxValue: number;

  setValueAtTime: (value: number, startTime: number) => void;
  linearRampToValueAtTime: (value: number, endTime: number) => void;
  exponentialRampToValueAtTime: (value: number, endTime: number) => void;
  setTargetAtTime: (
    target: number,
    startTime: number,
    timeConstant: number
  ) => void;
  setValueCurveAtTime: (
    values: Float32Array,
    startTime: number,
    duration: number
  ) => void;
  cancelScheduledValues: (cancelTime: number) => void;
  cancelAndHoldAtTime: (cancelTime: number) => void;
}

export interface IPeriodicWave {}

export interface IAnalyserNode extends IAudioNode {
  fftSize: number;
  readonly frequencyBinCount: number;
  minDecibels: number;
  maxDecibels: number;
  smoothingTimeConstant: number;
  window: WindowType;

  getFloatFrequencyData: (array: Float32Array) => void;
  getByteFrequencyData: (array: Uint8Array) => void;
  getFloatTimeDomainData: (array: Float32Array) => void;
  getByteTimeDomainData: (array: Uint8Array) => void;
}

export interface IRecorderAdapterNode extends IAudioNode {}

export interface IWorkletNode extends IAudioNode {}

export interface IWorkletSourceNode extends IAudioScheduledSourceNode {}

export interface IWorkletProcessingNode extends IAudioNode {}

export interface IAudioRecorder {
  start: () => void;
  stop: () => void;
  connect: (node: IRecorderAdapterNode) => void;
  disconnect: () => void;

  // passing subscriptionId(uint_64 in cpp, string in js) to the cpp
  onAudioReady: string;
}

export interface IAudioDecoder {
  decodeWithMemoryBlock: (
    arrayBuffer: ArrayBuffer,
    sampleRate?: number
  ) => Promise<IAudioBuffer>;
  decodeWithFilePath: (
    sourcePath: string,
    sampleRate?: number
  ) => Promise<IAudioBuffer>;
  decodeWithPCMInBase64: (
    b64: string,
    inputSampleRate: number,
    inputChannelCount: number,
    interleaved?: boolean
  ) => Promise<IAudioBuffer>;
}

export interface IAudioStretcher {
  changePlaybackSpeed: (
    arrayBuffer: AudioBuffer,
    playbackSpeed: number
  ) => Promise<IAudioBuffer>;
}

export interface IAudioEventEmitter {
  addAudioEventListener<Name extends AudioEventName>(
    name: Name,
    callback: AudioEventCallback<Name>
  ): string;
  removeAudioEventListener<Name extends AudioEventName>(
    name: Name,
    subscriptionId: string
  ): void;
}
