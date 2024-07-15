import Oscillator from './Oscillator/Oscillator';

export class AudioContext {
  constructor() {}

  public createOscillator(frequency: number) {
    return Oscillator(frequency);
  }
}
