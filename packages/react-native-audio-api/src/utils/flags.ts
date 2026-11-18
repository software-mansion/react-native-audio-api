import { NativeAudioAPIModule } from '../specs';

/**
 * Internal: whether the native build includes FFmpeg (the `enableFFmpeg` build
 * option, off by default). Remote URL streaming, HLS and remote URL metadata
 * branch on it; batch decoding, encoding and concatenation do not need FFmpeg.
 * Not exported from the package.
 */
export function isFfmpegEnabled(): boolean {
  return NativeAudioAPIModule.isFfmpegEnabled();
}
