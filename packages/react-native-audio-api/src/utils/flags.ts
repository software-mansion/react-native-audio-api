import { NativeAudioAPIModule } from '../specs';

/**
 * Returns whether the native build includes FFmpeg.
 *
 * When `false`, remote URL streaming / HLS and remote URL metadata are
 * unavailable. Batch decoding, encoding, and concatenation use OS APIs /
 * miniaudio and do not require FFmpeg. See the runtime flags docs for the full
 * feature matrix.
 */
export function isFfmpegEnabled(): boolean {
  return NativeAudioAPIModule.isFfmpegEnabled();
}
