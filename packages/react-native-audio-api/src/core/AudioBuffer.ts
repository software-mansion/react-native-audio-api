import { IAudioBuffer } from '../interfaces';
import { IndexSizeError, NotSupportedError } from '../errors';
import { AudioBufferOptions } from '../types';

export default class AudioBuffer {
  readonly length: number;
  readonly duration: number;
  readonly sampleRate: number;
  readonly numberOfChannels: number;
  /** @internal */
  public readonly buffer: IAudioBuffer;

  constructor(options: AudioBufferOptions);

  /** @internal */
  constructor(buffer: IAudioBuffer);

  /** @internal */
  constructor(arg: AudioBufferOptions | IAudioBuffer) {
    this.buffer = this.isAudioBuffer(arg)
      ? arg
      : AudioBuffer.createBufferFromOptions(arg);
    this.length = this.buffer.length;
    this.duration = this.buffer.duration;
    this.sampleRate = this.buffer.sampleRate;
    this.numberOfChannels = this.buffer.numberOfChannels;
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

  private static createBufferFromOptions(
    options: AudioBufferOptions
  ): IAudioBuffer {
    const { numberOfChannels = 1, length, sampleRate } = options;
    if (numberOfChannels < 1 || numberOfChannels >= 32) {
      throw new NotSupportedError(
        `The number of channels provided (${numberOfChannels}) is outside the range [1, 32]`
      );
    }
    if (length <= 0) {
      throw new NotSupportedError(
        `The number of frames provided (${length}) is less than or equal to the minimum bound (0)`
      );
    }
    if (sampleRate < 8000 || sampleRate > 96000) {
      throw new NotSupportedError(
        `The sample rate provided (${sampleRate}) is outside the range [8000, 96000]`
      );
    }
    return global.createAudioBuffer(numberOfChannels, length, sampleRate);
  }

  private isAudioBuffer(obj: unknown): obj is IAudioBuffer {
    return (
      typeof obj === 'object' &&
      obj !== null &&
      'getChannelData' in obj &&
      typeof (obj as IAudioBuffer).getChannelData === 'function'
    );
  }
}
