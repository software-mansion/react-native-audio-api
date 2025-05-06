import {
  WindowType,
  ContextState,
  OscillatorType,
  BiquadFilterType,
  ChannelCountMode,
  AudioRecorderStatus,
  ChannelInterpretation,
} from './types';
import { AudioEventName, AudioEventCallback } from './events/types';

export interface IBaseAudioContext {
  readonly destination: IAudioDestinationNode;
  readonly state: ContextState;
  readonly sampleRate: number;
  readonly currentTime: number;

  createOscillator(): IOscillatorNode;
  createGain(): IGainNode;
  createStereoPanner(): IStereoPannerNode;
  createBiquadFilter: () => IBiquadFilterNode;
  createBufferSource: (pitchCorrection: boolean) => IAudioBufferSourceNode;
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
  decodeAudioDataSource: (sourcePath: string) => Promise<IAudioBuffer>;
  decodeAudioData: (arrayBuffer: ArrayBuffer) => Promise<IAudioBuffer>;
}

export interface IAudioContext extends IBaseAudioContext {
  close(): Promise<void>;
  resume(): Promise<void>;
  suspend(): Promise<void>;
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

  connect: (node: IAudioNode) => void;
  disconnect: (node?: IAudioNode) => void;
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
  start(when?: number): void;
  stop: (when: number) => void;
  onended: string;
}

export interface IOscillatorNode extends IAudioScheduledSourceNode {
  readonly frequency: IAudioParam;
  readonly detune: IAudioParam;
  type: OscillatorType;

  setPeriodicWave(periodicWave: IPeriodicWave): void;
}

export interface IAudioBufferSourceNode extends IAudioScheduledSourceNode {
  buffer: IAudioBuffer | null;
  loop: boolean;
  loopStart: number;
  loopEnd: number;
  detune: IAudioParam;
  playbackRate: IAudioParam;

  start: (when?: number, offset?: number, duration?: number) => void;
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

export type IAudioReadyCallback = (
  buffer: IAudioBuffer,
  numFrames: number,
  when: number
) => void;

export type IErrorCallback = (error: Error) => void;

export type IStatusChangeCallback = (
  status: AudioRecorderStatus,
  previousStatus: AudioRecorderStatus
) => void;

export interface IAudioRecorder {
  start: () => void;
  stop: () => void;

  onAudioReady: (callback: IAudioReadyCallback) => void;
  onError: (callback: IErrorCallback) => void;
  onStatusChange: (callback: IStatusChangeCallback) => void;
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
