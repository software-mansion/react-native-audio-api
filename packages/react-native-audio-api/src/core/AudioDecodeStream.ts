import { IAudioDecodeStream } from '../jsi-interfaces';
import {
  assertValidDecodeStreamFrameCount,
  assertValidDecodeStreamPosition,
} from '../utils/validation';
import AudioBuffer from './AudioBuffer';

/**
 * An audio source kept open to be decoded a few frames at a time, instead of as
 * one `AudioBuffer`. Memory follows the size of a read, not the duration of the
 * source.
 *
 * Reads are sequential: each one continues where the previous one stopped, so
 * the chunks of a stream, put end to end, hold the samples `decodeAudioData`
 * returns for the same source and sample rate.
 */
export default class AudioDecodeStream {
  /** Sample rate of the decoded audio, in Hz. */
  readonly sampleRate: number;
  readonly numberOfChannels: number;
  /** Duration reported by the source, in seconds; 0 when it is unknown. */
  readonly duration: number;

  /** @internal */
  private readonly stream: IAudioDecodeStream;

  /** @internal */
  constructor(stream: IAudioDecodeStream) {
    this.stream = stream;
    this.sampleRate = stream.sampleRate;
    this.numberOfChannels = stream.numberOfChannels;
    this.duration = stream.duration;
  }

  /**
   * Decodes the next `frameCount` frames.
   *
   * Resolves with a buffer of `frameCount` frames, shorter only when the source
   * ends inside it, and with `null` once the source has ended. Reads run one at
   * a time, in the order they were requested.
   *
   * @throws RangeError When `frameCount` is not a positive integer.
   */
  public async read(frameCount: number): Promise<AudioBuffer | null> {
    assertValidDecodeStreamFrameCount(frameCount);

    const buffer = await this.stream.read(frameCount);
    return buffer === null ? null : new AudioBuffer(buffer);
  }

  /**
   * Moves the read position to `seconds`. Compressed formats seek to the
   * nearest position they can decode from, so the next read may start a few
   * frames away from it.
   *
   * @throws RangeError When `seconds` is negative or not finite.
   */
  public async seek(seconds: number): Promise<void> {
    assertValidDecodeStreamPosition(seconds);

    await this.stream.seek(seconds);
  }

  /**
   * Releases the decoder once the reads already requested have settled. Reads
   * and seeks requested afterwards reject.
   */
  public close(): void {
    this.stream.close();
  }
}
