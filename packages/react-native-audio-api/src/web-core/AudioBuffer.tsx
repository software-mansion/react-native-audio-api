import { IndexSizeError } from '../errors';

export default class AudioBuffer {
  readonly length: number;
  readonly duration: number;
  readonly sampleRate: number;
  readonly numberOfChannels: number;

  /** @internal */
  public readonly buffer: globalThis.AudioBuffer;

  constructor(buffer: globalThis.AudioBuffer) {
    this.buffer = buffer;
    this.length = buffer.length;
    this.duration = buffer.duration;
    this.sampleRate = buffer.sampleRate;
    this.numberOfChannels = buffer.numberOfChannels;
  }

  public getChannelData(channel: number): number[] {
    if (channel < 0 || channel >= this.numberOfChannels) {
      throw new IndexSizeError(
        `The channel number provided (${channel}) is outside the range [0, ${this.numberOfChannels - 1}]`
      );
    }

    return Array.from(this.buffer.getChannelData(channel));
  }

  public copyFromChannel(
    destination: number[],
    channelNumber: number,
    startInChannel: number = 0
  ): void {
    if (channelNumber < 0 || channelNumber >= this.numberOfChannels) {
      throw new IndexSizeError(
        `The channel number provided (${channelNumber}) is outside the range [0, ${this.numberOfChannels - 1}]`
      );
    }

    if (startInChannel < 0 || startInChannel >= this.length) {
      throw new IndexSizeError(
        `The startInChannel number provided (${startInChannel}) is outside the range [0, ${this.length - 1}]`
      );
    }

    const array = new Float32Array(destination);

    this.buffer.copyFromChannel(array, channelNumber, startInChannel);

    for (let i = 0; i < destination.length; i++) {
      destination[i] = array[i];
    }
  }

  public copyToChannel(
    source: number[],
    channelNumber: number,
    startInChannel: number = 0
  ): void {
    if (channelNumber < 0 || channelNumber >= this.numberOfChannels) {
      throw new IndexSizeError(
        `The channel number provided (${channelNumber}) is outside the range [0, ${this.numberOfChannels - 1}]`
      );
    }

    if (startInChannel < 0 || startInChannel >= this.length) {
      throw new IndexSizeError(
        `The startInChannel number provided (${startInChannel}) is outside the range [0, ${this.length - 1}]`
      );
    }

    this.buffer.copyToChannel(
      new Float32Array(source),
      channelNumber,
      startInChannel
    );
  }
}
