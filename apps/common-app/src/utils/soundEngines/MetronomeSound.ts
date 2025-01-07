import { AudioContext } from 'react-native-audio-api';

import type { SoundEngine } from './SoundEngine';

class MetronomeSound implements SoundEngine {
  public audioContext: AudioContext;
  public tone: number;
  public decay: number;
  public volume: number;

  constructor(audioContext: AudioContext, tone: number) {
    this.audioContext = audioContext;
    this.tone = tone;
    this.decay = 0.03;
    this.volume = 1;
  }

  play(time: number) {
    const oscillator = this.audioContext.createOscillator();
    const gain = this.audioContext.createGain();

    oscillator.frequency.value = this.tone;

    gain.gain.setValueAtTime(this.volume, time);
    gain.gain.linearRampToValueAtTime(0.5, time + 0.001);
    gain.gain.linearRampToValueAtTime(0.001, time + this.decay);
    gain.gain.setValueAtTime(0, time + this.decay + 0.001);

    oscillator.connect(gain);
    gain.connect(this.audioContext.destination);

    oscillator.start(time);
    oscillator.stop(time + this.decay);
  }
}

export default MetronomeSound;
