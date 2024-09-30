import { AudioContext } from 'react-native-audio-api';

import type { SoundEngine } from './SoundEngine';

class HiHat implements SoundEngine {
  public audioContext: AudioContext;
  public tone: number;
  public decay: number;
  public volume: number;
  private ratios: number[];
  private bandpassFilterFrequency: number;
  private highpassFilterFrequency: number;

  constructor(audioContext: AudioContext) {
    this.audioContext = audioContext;
    this.tone = 40;
    this.decay = 0.5;
    this.volume = 1;
    this.ratios = [2, 3, 4.16, 5.43, 6.79, 8.21];
    this.bandpassFilterFrequency = 10000;
    this.highpassFilterFrequency = 7000;
  }

  play(time: number) {
    const bandpassFilter = this.audioContext.createBiquadFilter();
    const highpassFilter = this.audioContext.createBiquadFilter();
    const gain = this.audioContext.createGain();

    bandpassFilter.type = 'bandpass';
    bandpassFilter.frequency.value = this.bandpassFilterFrequency;

    highpassFilter.type = 'highpass';
    highpassFilter.frequency.value = this.highpassFilterFrequency;

    gain.gain.setValueAtTime(0.0001, time);
    gain.gain.exponentialRampToValueAtTime(this.volume, time + 0.02);
    gain.gain.exponentialRampToValueAtTime(this.volume * 0.33, time + 0.03);
    gain.gain.exponentialRampToValueAtTime(this.volume * 0.0001, time + 0.3);

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
  }
}

export default HiHat;
