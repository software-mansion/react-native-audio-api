import {
  GainNode,
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
} from 'react-native-audio-api';

import { Key, getSource } from './utils';

class PianoNote {
  public audioContext: AudioContext;
  public key: Key;

  private gain: GainNode | null = null;
  private bufferSource: AudioBufferSourceNode | null = null;
  private startedAt: number | null = null;

  constructor(audioContext: AudioContext, key: Key) {
    this.audioContext = audioContext;
    this.key = key;
  }

  start(bufferList: Record<string, AudioBuffer>) {
    const { buffer, playbackRate } = getSource(bufferList, this.key);

    if (!buffer) {
      return;
    }

    const tNow = this.audioContext.currentTime;

    this.bufferSource = this.audioContext.createBufferSource();
    this.bufferSource.buffer = buffer;
    this.bufferSource.playbackRate.value = playbackRate;

    this.gain = this.audioContext.createGain();
    this.gain.gain.setValueAtTime(1, this.audioContext.currentTime);

    this.bufferSource.connect(this.gain);
    this.gain.connect(this.audioContext.destination);

    this.bufferSource.start(tNow);
    this.startedAt = tNow;
  }

  stop() {
    if (!this.bufferSource || !this.gain) {
      return;
    }

    const tNow = Math.max(
      this.audioContext.currentTime,
      (this.startedAt ?? 0) + 5.0
    );

    this.gain.gain.exponentialRampToValueAtTime(0.0001, tNow + 0.08);
    this.gain.gain.setValueAtTime(0, tNow + 0.09);
    this.bufferSource.stop(tNow + 0.1);

    this.bufferSource = null;
    this.gain = null;
  }
}

export default PianoNote;
