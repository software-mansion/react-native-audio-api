import {
  AudioContext,
  type OscillatorNode,
  type GainNode,
} from 'react-native-audio-context';

import type { SoundEngine } from './SoundEngine';

export class Kick implements SoundEngine {
  public audioContext: AudioContext;
  public tone: number;
  public decay: number;
  public volume: number;
  private gain!: GainNode;
  private oscillator!: OscillatorNode;

  constructor(audioContext: AudioContext) {
    this.audioContext = audioContext;
    this.tone = 167;
    this.decay = 0.5;
    this.volume = 1;
  }

  setup() {
    this.gain = this.audioContext.createGain();
    this.oscillator = this.audioContext.createOscillator();
    this.oscillator.connect(this.gain);
    this.gain.connect(this.audioContext.destination);
  }

  play(time: number) {
    this.setup();

    this.oscillator.frequency.setValueAtTime(this.tone, time);
    this.oscillator.frequency.exponentialRampToValueAtTime(
      0.01,
      time + this.decay
    );

    this.gain.gain.setValueAtTime(this.volume, time);
    this.gain.gain.exponentialRampToValueAtTime(0.01, time + this.decay);

    this.oscillator.start(time);
    this.oscillator.stop(time + this.decay);
  }
}
