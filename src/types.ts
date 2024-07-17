export interface BaseAudioContext {
  createOscillator(frequency: number): Oscillator;
  destination(): AudioDestinationNode;
}

export interface AudioNode {
  context: BaseAudioContext;
  connect: (destination: AudioDestinationNode) => void;
  disconnect: () => void;
}

export interface AudioDestinationNode extends AudioNode {}

export interface AudioScheduledSourceNode extends AudioNode {
  start: () => void;
  stop: () => void;
}

type WaveType = 'sine' | 'square' | 'sawtooth' | 'triangle';

export interface Oscillator extends AudioScheduledSourceNode {
  frequency: number;
  type: WaveType;
  detune: number;
}
