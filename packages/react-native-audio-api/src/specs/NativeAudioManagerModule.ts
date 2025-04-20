import { NativeModules, NativeEventEmitter, Platform } from 'react-native';

const NativeAudioManagerModule = NativeModules.AudioManagerModule;
const eventEmitter =
  Platform.OS === 'android'
    ? new NativeEventEmitter()
    : new NativeEventEmitter(NativeModules.AudioManagerModule);

if (!NativeAudioManagerModule || !eventEmitter) {
  throw new Error(
    `Failed to install react-native-audio-api: The native module could not be found.`
  );
}

const AudioManagerModule = {
  setLockScreenInfo(info: {
    [key: string]: string | boolean | number | undefined;
  }): void {
    NativeAudioManagerModule.setLockScreenInfo(info);
  },
  resetLockScreenInfo(): void {
    NativeAudioManagerModule.resetLockScreenInfo();
  },
  enableRemoteCommand(name: string, enabled: boolean): void {
    NativeAudioManagerModule.enableRemoteCommand(name, enabled);
  },
  setAudioSessionOptions(
    category: string,
    mode: string,
    options: Array<string>
  ): void {
    NativeAudioManagerModule.setAudioSessionOptions(category, mode, options);
  },
  getDevicePreferredSampleRate(): number {
    return NativeAudioManagerModule.getDevicePreferredSampleRate();
  },
  observeAudioInterruptions(enabled: boolean): void {
    NativeAudioManagerModule.observeAudioInterruptions(enabled);
  },
};

export { eventEmitter, AudioManagerModule };
