import { IAudioBuffer } from '../interfaces';

export default class AudioBuffer {
  readonly length: number;
  readonly duration: number;
  readonly sampleRate: number;
  readonly numberOfChannels: number;
  public readonly buffer: IAudioBuffer;

  constructor(buffer: IAudioBuffer) {
    this.buffer = buffer;
    this.length = buffer.length;
    this.duration = buffer.duration;
    this.sampleRate = buffer.sampleRate;
    this.numberOfChannels = buffer.numberOfChannels;
  }

  public getChannelData(channel: number): number[] {
    return this.buffer.getChannelData(channel);
  }

  public setChannelData(channel: number, data: number[]): void {
    this.buffer.setChannelData(channel, data);
  }
}
