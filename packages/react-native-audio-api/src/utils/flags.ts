import { NativeAudioAPIModule } from '../specs';

/**
 * Returns whether the native build includes FFmpeg. FFmpeg is an opt-in build
 * option, `enableFFmpeg`, set in the Expo plugin options or, in a bare React
 * Native app, in the `"react-native-audio-api"` section of the app's
 * package.json. This is `false` unless the app turned it on.
 *
 * When `false`, remote URL streaming / HLS and remote URL metadata are
 * unavailable. Batch decoding, encoding, and concatenation use OS APIs /
 * miniaudio and do not require FFmpeg. See the runtime flags docs for the full
 * feature matrix.
 */
export function isFfmpegEnabled(): boolean {
  return NativeAudioAPIModule.isFfmpegEnabled();
}
