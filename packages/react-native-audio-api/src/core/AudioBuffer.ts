import { AudioBufferLike, AudioBufferOptions } from '../types';
import { IAudioBuffer } from '../jsi-interfaces';
import {
  IndexSizeError,
  NotSupportedError,
  wrapFloat32ArrayView,
} from '../errors';
import { assertSupportedSampleRate } from '../utils/validation';

export default class AudioBuffer implements AudioBufferLike {
  readonly length: number;
  readonly duration: number;
  readonly sampleRate: number;
  readonly numberOfChannels: number;
  /** @internal */
  public readonly buffer: IAudioBuffer;

  // Per the Web Audio spec, getChannelData() must return the same Float32Array
  // for a given channel across calls (`buffer.getChannelData(0) === ...`). The
  // native getChannelData creates a fresh view each time, so cache it here.
  private readonly channelDataCache: (Float32Array<ArrayBuffer> | undefined)[] =
    [];

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

  public getChannelData(channel: number): Float32Array<ArrayBuffer> {
    if (channel < 0 || channel >= this.numberOfChannels) {
      throw new IndexSizeError(
        `The channel number provided (${channel}) is outside the range [0, ${this.numberOfChannels - 1}]`
      );
    }

    const cached = this.channelDataCache[channel];
    if (cached !== undefined) {
      return cached;
    }

    const data = wrapFloat32ArrayView(
      this.buffer.getChannelData(channel)
    ) as Float32Array<ArrayBuffer>;
    this.channelDataCache[channel] = data;
    return data;
  }

  public copyFromChannel(
    destination: Float32Array<ArrayBuffer>,
    channelNumber: number,
    startInChannel: number = 0
  ): void {
    if (channelNumber < 0 || channelNumber >= this.numberOfChannels) {
      throw new IndexSizeError(
        `The channel number provided (${channelNumber}) is outside the range [0, ${this.numberOfChannels - 1}]`
      );
    }

    // Per spec, an out-of-range startInChannel copies nothing rather than
    // throwing; the native layer clamps the copy length.
    this.buffer.copyFromChannel(destination, channelNumber, startInChannel);
  }

  public copyToChannel(
    source: Float32Array<ArrayBuffer>,
    channelNumber: number,
    startInChannel: number = 0
  ): void {
    if (channelNumber < 0 || channelNumber >= this.numberOfChannels) {
      throw new IndexSizeError(
        `The channel number provided (${channelNumber}) is outside the range [0, ${this.numberOfChannels - 1}]`
      );
    }

    // Per spec, an out-of-range startInChannel copies nothing rather than
    // throwing; the native layer clamps the copy length.
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
    assertSupportedSampleRate(sampleRate);
    return globalThis.createAudioBuffer(numberOfChannels, length, sampleRate);
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
