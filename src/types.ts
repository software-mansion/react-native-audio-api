export interface BaseAudioContext {
  destination: AudioDestinationNode | null;
  createOscillator(): Oscillator;
}

export interface AudioNode {
  context: BaseAudioContext;
  connect: (destination: AudioDestinationNode) => void;
  disconnect: () => void;
}

export interface AudioDestinationNode extends AudioNode {}

export interface AudioScheduledSourceNode extends AudioNode {
  start: (time: number) => void;
  stop: (time: number) => void;
}

type WaveType = 'sine' | 'square' | 'sawtooth' | 'triangle';

export interface Oscillator extends AudioScheduledSourceNode {
  frequency: number;
  type: WaveType;
  detune: number;
}
