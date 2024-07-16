export class AudioContext {
  public createOscillator(frequency: number) {
    return global.createOscillator(frequency);
  }
}
