import type {
  AudioDestinationNode,
  BaseAudioContext,
  Oscillator,
} from './types';

export class AudioContext implements BaseAudioContext {
  constructor() {}

  public createOscillator(frequency: number): Oscillator {
    return global.__AudioContextProxy.createOscillator(frequency);
  }

  public destination(): AudioDestinationNode {
    throw new Error('Method not implemented.');
  }
}
