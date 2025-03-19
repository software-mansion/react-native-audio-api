import { SessionOptions, LockScreenInfo } from './types';
import NativeAudioManagerModule from '../specs/NativeAudioManagerModule';

if (!NativeAudioManagerModule) {
  throw new Error(
    `Failed to install react-native-audio-api: The native module could not be found.`
  );
}

class AudioManager {
  setOptions(options: SessionOptions) {
    NativeAudioManagerModule.setSessionCategory(options.iosCategory || '');
    NativeAudioManagerModule.setSessionMode(options.iosMode || '');
    NativeAudioManagerModule.setSessionCategoryOptions(
      options.iosOptions || []
    );
  }

  setNowPlaying(info: LockScreenInfo) {
    NativeAudioManagerModule.setNowPlaying(info);
  }

  getSampleRate(): number {
    return NativeAudioManagerModule.getSampleRate();
  }
}

export default new AudioManager();
