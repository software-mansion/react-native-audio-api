export interface BaseAudioContext {
  readonly destination: AudioDestinationNode;
  readonly state: ContextState;
  readonly sampleRate: number;
  readonly currentTime: number;
  createOscillator(): OscillatorNode;
  createGain(): GainNode;
  createStereoPanner(): StereoPannerNode;
  createBiquadFilter: () => BiquadFilterNode;
  createBufferSource: () => AudioBufferSourceNode;
  createBuffer: (
    channels: number,
    length: number,
    sampleRate: number
  ) => AudioBuffer;
  close(): void;
}

type channelCountMode = 'max' | 'clamped-max' | 'explicit';

type channelInterpretation = 'speakers' | 'discrete';

export interface AudioNode {
  readonly context: BaseAudioContext;
  readonly numberOfInputs: number;
  readonly numberOfOutputs: number;
  readonly channelCount: number;
  readonly channelCountMode: channelCountMode;
  readonly channelInterpretation: channelInterpretation;
  connect: (node: AudioNode) => void;
  disconnect: (node: AudioNode) => void;
}

export interface AudioParam {
  value: number;
  defaultValue: number;
  minValue: number;
  maxValue: number;
  setValueAtTime: (value: number, startTime: number) => void;
  linearRampToValueAtTime: (value: number, endTime: number) => void;
  exponentialRampToValueAtTime: (value: number, endTime: number) => void;
}

export interface AudioDestinationNode extends AudioNode {}

export interface AudioScheduledSourceNode extends AudioNode {
  start: (time: number) => void;
  stop: (time: number) => void;
}

export type WaveType = 'sine' | 'square' | 'sawtooth' | 'triangle';

export interface OscillatorNode extends AudioScheduledSourceNode {
  frequency: AudioParam;
  type: WaveType;
  detune: AudioParam;
}

export interface GainNode extends AudioNode {
  gain: AudioParam;
}

export interface StereoPannerNode extends AudioNode {
  pan: AudioParam;
}

type FilterType =
  | 'lowpass'
  | 'highpass'
  | 'bandpass'
  | 'lowshelf'
  | 'highshelf'
  | 'peaking'
  | 'notch'
  | 'allpass';

export interface BiquadFilterNode extends AudioNode {
  frequency: AudioParam;
  detune: AudioParam;
  Q: AudioParam;
  gain: AudioParam;
  type: FilterType;
}

export type ContextState = 'running' | 'closed' | 'suspended';

export interface AudioBuffer {
  readonly length: number;
  readonly duration: number;
  readonly sampleRate: number;
  readonly numberOfChannels: number;
  getChannelData(channel: number): number[];
  copyFromChannel(
    destination: number[],
    channelNumber: number,
    startInChannel: number
  ): void;
  copyToChannel(
    source: number[],
    channelNumber: number,
    startInChannel: number
  ): void;
}

export interface AudioBufferSourceNode extends AudioScheduledSourceNode {
  buffer: AudioBuffer;
  loop: boolean;
}
