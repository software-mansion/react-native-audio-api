import { AudioBufferLike, AudioBufferOptions } from '../types';
import { IAudioBuffer } from '../jsi-interfaces';
import {
  IndexSizeError,
  NotSupportedError,
  wrapFloat32ArrayView,
} from '../errors';

export default class AudioBuffer implements AudioBufferLike {
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

  public getChannelData(channel: number): Float32Array<ArrayBuffer> {
    if (channel < 0 || channel >= this.numberOfChannels) {
      throw new IndexSizeError(
        `The channel number provided (${channel}) is outside the range [0, ${this.numberOfChannels - 1}]`
      );
    }
    return wrapFloat32ArrayView(
      this.buffer.getChannelData(channel)
    ) as Float32Array<ArrayBuffer>;
  }

  public copyFromChannel(
    destination: Float32Array<ArrayBuffer>,
    channelNumber: number,
    startInChannel: number = 0
  ): void {
    AudioBuffer.assertFloat32Array(destination, 'destination');
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
    source: Float32Array<ArrayBuffer>,
    channelNumber: number,
    startInChannel: number = 0
  ): void {
    AudioBuffer.assertFloat32Array(source, 'source');
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

  private static assertFloat32Array(value: unknown, name: string): void {
    // Cross-realm Float32Arrays (e.g. from a jsdom window) fail instanceof,
    // so also accept any object that looks like a Float32Array view.
    const isFloat32View =
      value instanceof Float32Array ||
      (typeof value === 'object' &&
        value !== null &&
        ArrayBuffer.isView(value as ArrayBufferView) &&
        (value as Float32Array).BYTES_PER_ELEMENT === 4 &&
        (value.constructor as { name?: string })?.name === 'Float32Array');

    if (!isFloat32View) {
      throw new TypeError(`The provided ${name} is not a Float32Array`);
    }

    const backingBuffer = (value as Float32Array).buffer as {
      constructor?: { name?: string };
    };
    if (backingBuffer?.constructor?.name === 'SharedArrayBuffer') {
      throw new TypeError(
        `The provided ${name} is backed by a SharedArrayBuffer, which is not allowed`
      );
    }
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
