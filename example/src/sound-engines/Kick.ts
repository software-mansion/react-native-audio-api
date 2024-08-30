import { AudioContext } from 'react-native-audio-context';
import type { SoundEngine } from './SoundEngine';

export class Kick implements SoundEngine {
  public audioContext: AudioContext;
  public tone: number;
  public decay: number;
  public volume: number;

  constructor(audioContext: AudioContext) {
    this.audioContext = audioContext;
    this.tone = 125;
    this.decay = 0.2;
    this.volume = 1;
  }

  play(time: number) {
    const oscillator = this.audioContext.createOscillator();
    const gain = this.audioContext.createGain();

    oscillator.connect(gain);
    gain.connect(this.audioContext.destination);

    oscillator.frequency.setValueAtTime(this.tone, time);
    oscillator.frequency.exponentialRampToValueAtTime(0.01, time + this.decay);

    gain.gain.setValueAtTime(this.volume, time);
    gain.gain.exponentialRampToValueAtTime(0.01, time + this.decay);

    oscillator.start(time);
    oscillator.stop(time + this.decay);
  }
}
