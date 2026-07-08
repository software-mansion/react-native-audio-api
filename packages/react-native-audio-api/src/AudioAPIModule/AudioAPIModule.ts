import { NativeAudioAPIModule } from '../specs';
import { AudioApiError } from '../errors';

class AudioAPIModule {
  constructor() {
    if (this.#verifyInstallation()) {
      return;
    }

    if (!NativeAudioAPIModule) {
      throw new AudioApiError(
        `Failed to install react-native-audio-api: The native module could not be found.`
      );
    }

    NativeAudioAPIModule.install();
  }

  #verifyInstallation(): boolean {
    return (
      globalThis.createAudioContext != null &&
      globalThis.createOfflineAudioContext != null &&
      globalThis.createAudioRecorder != null &&
      globalThis.createAudioBuffer != null &&
      globalThis.createAudioDecoder != null &&
      globalThis.createAudioFileUtils != null &&
      globalThis.AudioEventEmitter != null
    );
  }
}

export default new AudioAPIModule();
