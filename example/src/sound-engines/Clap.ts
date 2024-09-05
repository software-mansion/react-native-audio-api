import { AudioContext } from 'react-native-audio-api';
import type { SoundEngine } from './SoundEngine';

const MAX_SHORT = 32767;

export class Clap implements SoundEngine {
  public audioContext: AudioContext;
  public tone: number;
  public decay: number;
  public volume: number;
  private pulseWidth: number;

  constructor(audioContext: AudioContext) {
    this.audioContext = audioContext;
    this.tone = 130;
    this.decay = 0.3;
    this.volume = 1;
    this.pulseWidth = 0.025;
  }

  createNoiseBuffer() {
    const bufferSize = 256;
    const buffer = this.audioContext.createBuffer(
      this.audioContext.sampleRate,
      256,
      1
    );
    const output = new Array<number>(bufferSize);

    for (let i = 0; i < bufferSize; i++) {
      output[i] = (Math.random() * 2 - 1) * MAX_SHORT;
    }

    buffer.setChannelData(0, output);

    return buffer;
  }

  play(time: number) {
    const envelope = this.audioContext.createGain();
    const noise = this.audioContext.createBufferSource();
    const filter = this.audioContext.createBiquadFilter();

    noise.buffer = this.createNoiseBuffer();
    filter.type = 'bandpass';
    filter.frequency.value = this.tone * 2;

    noise.connect(filter);
    filter.connect(envelope);
    envelope.connect(this.audioContext.destination!);

    envelope.gain.setValueAtTime(this.volume, time);
    envelope.gain.exponentialRampToValueAtTime(0.01, time + this.pulseWidth);

    envelope.gain.setValueAtTime(this.volume, time);
    envelope.gain.exponentialRampToValueAtTime(
      0.01,
      time + 2 * this.pulseWidth
    );

    envelope.gain.setValueAtTime(this.volume, time + 2 * this.pulseWidth);
    envelope.gain.exponentialRampToValueAtTime(0.001, time + this.decay);

    noise.start(time);
    noise.stop(time + this.decay);
  }
}
