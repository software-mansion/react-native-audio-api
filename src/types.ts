export interface BaseAudioContext {
  readonly destination: AudioDestinationNode;
  readonly state: ContextState;
  readonly sampleRate: number;
  createOscillator(): OscillatorNode;
  createGain(): GainNode;
  createStereoPanner(): StereoPannerNode;
}

export interface AudioNode {
  readonly context: BaseAudioContext;
  readonly numberOfInputs: number;
  readonly numberOfOutputs: number;
  connect: (node: AudioNode) => void;
  disconnect: (node: AudioNode) => void;
}

export interface AudioParam {
  value: number;
  defaultValue: number;
  minValue: number;
  maxValue: number;
}

export interface AudioDestinationNode extends AudioNode {}

export interface AudioScheduledSourceNode extends AudioNode {
  start: (time: number) => void;
  stop: (time: number) => void;
}

type WaveType = 'sine' | 'square' | 'sawtooth' | 'triangle';

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

export type ContextState = 'running' | 'closed';
