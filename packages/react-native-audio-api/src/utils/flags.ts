import { NativeAudioAPIModule } from '../specs';

/**
 * Returns whether the native build includes FFmpeg.
 *
 * When `false`, streaming, remote URL metadata, and several decode/playback
 * paths are unavailable.
 */
export function isFfmpegEnabled(): boolean {
  return NativeAudioAPIModule.isFfmpegEnabled();
}
