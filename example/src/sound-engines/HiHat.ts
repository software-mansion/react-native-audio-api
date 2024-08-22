import {
  AudioContext,
  type GainNode,
  type BiquadFilterNode,
} from 'react-native-audio-context';

import type { SoundEngine } from './SoundEngine';

export class HiHat implements SoundEngine {
  public audioContext: AudioContext;
  public tone: number;
  public decay: number;
  public volume: number;
  private bandpassFilter!: BiquadFilterNode;
  private highpassFilter!: BiquadFilterNode;
  private gain!: GainNode;
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

  setup() {
    this.bandpassFilter = this.audioContext.createBiquadFilter();
    this.highpassFilter = this.audioContext.createBiquadFilter();
    this.gain = this.audioContext.createGain();

    this.bandpassFilter.type = 'bandpass';
    this.bandpassFilter.frequency.value = this.bandpassFilterFrequency;
    this.bandpassFilter.Q.value = this.bandPassQ;

    this.highpassFilter.type = 'highpass';
    this.highpassFilter.frequency.value = this.highpassFilterFrequency;

    this.bandpassFilter.connect(this.highpassFilter);
    this.highpassFilter.connect(this.gain);
    this.gain.connect(this.audioContext.destination!);
  }

  play(time: number) {
    this.setup();

    this.ratios.forEach((ratio) => {
      const oscillator = this.audioContext.createOscillator();
      oscillator.type = 'square';
      oscillator.frequency.value = this.tone * ratio;
      oscillator.connect(this.bandpassFilter);
      oscillator.start(time);
      oscillator.stop(time + this.decay);
    });

    this.gain.gain.exponentialRampToValueAtTime(
      this.volume,
      time + 0.067 * this.decay
    );
    this.gain.gain.exponentialRampToValueAtTime(
      this.volume * 0.3,
      time + 0.1 * this.decay
    );
    this.gain.gain.exponentialRampToValueAtTime(
      this.volume * 0.00001,
      time + this.decay
    );
  }
}
