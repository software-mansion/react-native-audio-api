import { AudioContext } from 'react-native-audio-context';
import type { SoundEngine } from './SoundEngine';

export class HiHat implements SoundEngine {
  public audioContext: AudioContext;
  public tone: number;
  public decay: number;
  public volume: number;
  private ratios: number[];
  private bandpassFilterFrequency: number;
  private highpassFilterFrequency: number;
  private bandPassQ: number;

  constructor(audioContext: AudioContext) {
    this.audioContext = audioContext;
    this.tone = 40;
    this.decay = 0.5;
    this.volume = 1;
    this.ratios = [2, 3, 4.16, 5.43, 6.79, 8.21];
    this.bandpassFilterFrequency = 10000;
    this.highpassFilterFrequency = 7000;
    this.bandPassQ = 0.2;
  }

  play(time: number) {
    const bandpassFilter = this.audioContext.createBiquadFilter();
    const highpassFilter = this.audioContext.createBiquadFilter();
    const gain = this.audioContext.createGain();

    bandpassFilter.type = 'bandpass';
    bandpassFilter.frequency.value = this.bandpassFilterFrequency;
    bandpassFilter.Q.value = this.bandPassQ;

    highpassFilter.type = 'highpass';
    highpassFilter.frequency.value = this.highpassFilterFrequency;

    bandpassFilter.connect(highpassFilter);
    highpassFilter.connect(gain);
    gain.connect(this.audioContext.destination!);

    this.ratios.forEach((ratio) => {
      const oscillator = this.audioContext.createOscillator();
      oscillator.type = 'square';
      oscillator.frequency.value = this.tone * ratio;
      oscillator.connect(bandpassFilter);
      oscillator.start(time);
      oscillator.stop(time + 0.3);
    });

    gain.gain.setValueAtTime(0.01, time);

    gain.gain.exponentialRampToValueAtTime(this.volume, time + 0.02);
    gain.gain.exponentialRampToValueAtTime(this.volume * 0.3, time + 0.03);
    gain.gain.exponentialRampToValueAtTime(this.volume * 0.001, time + 0.3);
    gain.gain.exponentialRampToValueAtTime(this.volume * 0, time + 0.31);
  }
}
