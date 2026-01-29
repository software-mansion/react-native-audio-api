import { IndexSizeError } from '../errors';
import BaseAudioContext from './BaseAudioContext';
import { TAudioBufferOptions } from '../types';

export default class AudioBuffer {
  readonly length: number;
  readonly duration: number;
  readonly sampleRate: number;
  readonly numberOfChannels: number;

  /** @internal */
  public readonly buffer: globalThis.AudioBuffer;

  constructor(buffer: globalThis.AudioBuffer);
  constructor(context: BaseAudioContext, options: TAudioBufferOptions);

  constructor(
    contextOrBuffer: BaseAudioContext | globalThis.AudioBuffer,
    options?: TAudioBufferOptions
  ) {
    let buf: globalThis.AudioBuffer;
    if (contextOrBuffer instanceof BaseAudioContext) {
      buf = new globalThis.AudioBuffer(options!);
    } else {
      buf = contextOrBuffer as globalThis.AudioBuffer;
    }
    this.buffer = buf;
    this.length = buf.length;
    this.duration = buf.duration;
    this.sampleRate = buf.sampleRate;
    this.numberOfChannels = buf.numberOfChannels;
  }

  public getChannelData(channel: number): Float32Array {
    if (channel < 0 || channel >= this.numberOfChannels) {
      throw new IndexSizeError(
        `The channel number provided (${channel}) is outside the range [0, ${this.numberOfChannels - 1}]`
      );
    }

    return this.buffer.getChannelData(channel);
  }

  public copyFromChannel(
    destination: Float32Array,
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

    this.buffer.copyFromChannel(destination, channelNumber, startInChannel);
  }

  public copyToChannel(
    source: Float32Array,
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

    this.buffer.copyToChannel(source, channelNumber, startInChannel);
  }
}
