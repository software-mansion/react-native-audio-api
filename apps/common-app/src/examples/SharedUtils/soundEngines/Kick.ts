import { AudioContext } from 'react-native-audio-api';

import type { SoundEngine } from './SoundEngine';

class Kick implements SoundEngine {
  public audioContext: AudioContext;
  public tone: number;
  public decay: number;
  public volume: number;

  constructor(audioContext: AudioContext) {
    this.audioContext = audioContext;
    this.tone = 164;
    this.decay = 0.2;
    this.volume = 1;
  }

  play(time: number) {
    const oscillator = this.audioContext.createOscillator();
    const gain = this.audioContext.createGain();

    oscillator.frequency.setValueAtTime(0, time);
    oscillator.frequency.setValueAtTime(this.tone, time + 0.01);
    oscillator.frequency.exponentialRampToValueAtTime(10, time + this.decay);

    gain.gain.setValueAtTime(0, time);
    gain.gain.setValueAtTime(this.volume, time + 0.01);
    gain.gain.exponentialRampToValueAtTime(0.001, time + this.decay);
    gain.gain.setValueAtTime(0, time + this.decay + 0.001);

    oscillator.connect(gain);
    gain.connect(this.audioContext.destination);

    oscillator.start(time);
    oscillator.stop(time + this.decay);
  }
}

export default Kick;
