import { AudioContext } from 'react-native-audio-context';
import type { SoundEngine } from './SoundEngine';
import { Platform } from 'react-native';

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
    this.tone = 131;
    this.decay = 0.5;
    this.volume = 1;
    this.ratios = [1, 1.342, 1.2312, 1.6532, 1.9523, 2.1523];
    this.bandpassFilterFrequency = 20000;
    this.highpassFilterFrequency = 5000;
    this.bandPassQ = 0.2;
  }

  play(time: number) {
    if (Platform.OS === 'android') {
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
        oscillator.stop(time + this.decay);
      });

      gain.gain.exponentialRampToValueAtTime(
        this.volume,
        time + 0.067 * this.decay
      );
      gain.gain.exponentialRampToValueAtTime(
        this.volume * 0.3,
        time + 0.1 * this.decay
      );
      gain.gain.exponentialRampToValueAtTime(
        this.volume * 0.00001,
        time + this.decay
      );
    }
  }
}
