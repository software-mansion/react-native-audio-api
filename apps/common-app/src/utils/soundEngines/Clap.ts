import { AudioContext } from 'react-native-audio-api';

import type { SoundEngine } from './SoundEngine';

class Clap implements SoundEngine {
  public audioContext: AudioContext;
  public tone: number;
  public decay: number;
  public volume: number;
  private pulseWidth: number;
  private pulseCount: number;

  constructor(audioContext: AudioContext) {
    this.audioContext = audioContext;
    this.tone = 820;
    this.volume = 1;
    this.decay = 0.2;
    this.pulseWidth = 0.008;
    this.pulseCount = 4;
  }

  createNoiseBuffer() {
    const bufferSize = this.audioContext.sampleRate / 5;
    const buffer = this.audioContext.createBuffer(
      1,
      bufferSize,
      this.audioContext.sampleRate
    );

    const output = new Array<number>(bufferSize);

    for (let i = 0; i < bufferSize; i++) {
      output[i] = Math.random() * 2 - 1;
    }

    buffer.copyToChannel(output, 0, 0);

    return buffer;
  }

  play(time: number) {
    // TODO: add delay node once implemented!
    const noise = this.audioContext.createBufferSource();
    noise.buffer = this.createNoiseBuffer();

    const filter = this.audioContext.createBiquadFilter();
    filter.type = 'bandpass';
    filter.frequency.value = this.tone * 2;
    filter.Q.value = 1;

    const envelope = this.audioContext.createGain();

    noise.connect(filter);
    filter.connect(envelope);
    envelope.connect(this.audioContext.destination);

    for (let i = 0; i < this.pulseCount - 1; i += 1) {
      envelope.gain.setValueAtTime(this.volume, time + i * this.pulseWidth);

      envelope.gain.exponentialRampToValueAtTime(
        0.1,
        time + (i + 1) * this.pulseWidth
      );
    }

    envelope.gain.setValueAtTime(
      this.volume,
      time + (this.pulseCount - 1) * this.pulseWidth
    );
    envelope.gain.exponentialRampToValueAtTime(0.001, time + this.decay);
    envelope.gain.setValueAtTime(0, time + this.decay + 0.001);

    noise.start(time);
    noise.stop(time + this.decay);
  }
}

export default Clap;
