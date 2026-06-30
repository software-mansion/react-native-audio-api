import { NativeAudioAPIModule } from '../specs';

/**
 * Returns whether the native build includes FFmpeg.
 *
 * When `false`, streaming, remote URL metadata, several decode/playback paths,
 * M4A concatenation, and Android non-WAV recording are unavailable. See the
 * runtime flags docs for the full feature matrix.
 */
export function isFfmpegEnabled(): boolean {
  return NativeAudioAPIModule.isFfmpegEnabled();
}
