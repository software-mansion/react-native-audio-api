import { AudioApiError } from '../errors';
import {
  assertValidDecodeStreamFrameCount,
  assertValidDecodeStreamPosition,
} from '../utils/validation';
import AudioBuffer from './AudioBuffer.web';

/**
 * Web counterpart of the native decode stream. A browser only decodes a source
 * as a whole, so the stream keeps the decoded audio and hands out slices of it:
 * the API is the native one, the memory saving is not.
 */
export default class AudioDecodeStream {
  readonly sampleRate: number;
  readonly numberOfChannels: number;
  readonly duration: number;

  private decoded: AudioBuffer | null;
  private position: number = 0;

  /** @internal */
  constructor(decoded: AudioBuffer) {
    this.decoded = decoded;
    this.sampleRate = decoded.sampleRate;
    this.numberOfChannels = decoded.numberOfChannels;
    this.duration = decoded.duration;
  }

  public read(frameCount: number): Promise<AudioBuffer | null> {
    // a throw inside the executor rejects the promise
    return new Promise((resolve) => {
      assertValidDecodeStreamFrameCount(frameCount);
      resolve(this.readChunk(frameCount));
    });
  }

  public seek(seconds: number): Promise<void> {
    return new Promise((resolve) => {
      assertValidDecodeStreamPosition(seconds);
      const decoded = this.openDecodedAudio('seek in');
      this.position = Math.min(
        Math.round(seconds * this.sampleRate),
        decoded.length
      );
      resolve();
    });
  }

  private readChunk(frameCount: number): AudioBuffer | null {
    const decoded = this.openDecodedAudio('read from');
    const length = Math.min(frameCount, decoded.length - this.position);
    if (length <= 0) {
      return null;
    }

    const chunk = new AudioBuffer({
      length,
      numberOfChannels: this.numberOfChannels,
      sampleRate: this.sampleRate,
    });
    for (let channel = 0; channel < this.numberOfChannels; channel += 1) {
      chunk.copyToChannel(
        decoded
          .getChannelData(channel)
          .subarray(this.position, this.position + length),
        channel
      );
    }
    this.position += length;
    return chunk;
  }

  private openDecodedAudio(action: 'read from' | 'seek in'): AudioBuffer {
    if (this.decoded === null) {
      throw new AudioApiError(`Cannot ${action} a closed decode stream`);
    }
    return this.decoded;
  }

  public close(): void {
    this.decoded = null;
  }
}
