import { IAudioBuffer } from '../interfaces';

export default class AudioBuffer {
  readonly length: number;
  readonly duration: number;
  readonly sampleRate: number;
  readonly numberOfChannels: number;
  /** @internal */
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

  public copyFromChannel(
    destination: number[],
    channelNumber: number,
    startInChannel: number
  ): void {
    this.buffer.copyFromChannel(destination, channelNumber, startInChannel);
  }

  public copyToChannel(
    source: number[],
    channelNumber: number,
    startInChannel: number
  ): void {
    this.buffer.copyToChannel(source, channelNumber, startInChannel);
  }
}
