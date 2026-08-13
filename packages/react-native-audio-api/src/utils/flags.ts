import { NativeAudioAPIModule } from '../specs';

/**
 * Returns whether the native build includes FFmpeg.
 *
 * When `false`, remote URL streaming / HLS, remote URL metadata, M4A
 * concatenation, and Android non-WAV recording are unavailable. Batch decoding
 * uses OS APIs / miniaudio and does not require FFmpeg. See the runtime flags
 * docs for the full feature matrix.
 */
export function isFfmpegEnabled(): boolean {
  return NativeAudioAPIModule.isFfmpegEnabled();
}
