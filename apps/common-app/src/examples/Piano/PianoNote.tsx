import type {
  GainNode,
  AudioContext,
  OscillatorNode,
} from 'react-native-audio-api';

import { Key } from './utils';

class PianoNote {
  public audioContext: AudioContext;
  public key: Key;

  private oscillator: OscillatorNode | null = null;
  private gain: GainNode | null = null;

  constructor(audioContext: AudioContext, key: Key) {
    this.audioContext = audioContext;
    this.key = key;
  }

  start() {
    const tNow = this.audioContext.currentTime;

    this.oscillator = this.audioContext.createOscillator();
    this.gain = this.audioContext.createGain();
    this.oscillator.type = 'triangle';

    this.oscillator.connect(this.gain);
    this.gain.connect(this.audioContext.destination);

    this.oscillator.frequency.value = this.key.frequency;

    this.gain.gain.setValueAtTime(0.001, this.audioContext.currentTime);
    this.gain.gain.exponentialRampToValueAtTime(1, tNow + 0.1);

    this.oscillator.start(tNow);
  }

  stop() {
    if (!this.oscillator || !this.gain) {
      return;
    }

    const tNow = this.audioContext.currentTime;

    this.gain.gain.setValueAtTime(1, tNow);
    this.gain.gain.exponentialRampToValueAtTime(0.001, tNow + 0.05);
    this.gain.gain.setValueAtTime(0, tNow + 0.1);

    this.oscillator.stop(tNow + 0.1);

    this.gain.disconnect(this.audioContext.destination);
    this.oscillator.disconnect(this.gain);

    this.oscillator = null;
    this.gain = null;
  }
}

export default PianoNote;
