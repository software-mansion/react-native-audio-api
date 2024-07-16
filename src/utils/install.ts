import { NativeModules, Platform } from 'react-native';

export function getAudioContextModule() {
  const AudioContextModule = NativeModules.AudioContext;
  if (AudioContextModule == null) {
    let message =
      'Failed to install react-native-audio-context: The native `Oscillator` Module could not be found.';
    message +=
      '\n* Make sure react-native-audio-context is correctly autolinked (run `npx react-native config` to verify)';
    if (Platform.OS === 'ios' || Platform.OS === 'macos') {
      message += '\n* Make sure you ran `pod install` in the ios/ directory.';
    }
    if (Platform.OS === 'android') {
      message += '\n* Make sure gradle is synced.';
    }
    message += '\n* Make sure you rebuilt the app.';
    throw new Error(message);
  }
  return AudioContextModule;
}

export function verifyExpoGo() {
  const ExpoConstants =
    NativeModules.NativeUnimoduleProxy?.modulesConstants?.ExponentConstants;
  if (ExpoConstants != null) {
    if (ExpoConstants.appOwnership === 'expo') {
      throw new Error(
        'react-native-audio-context is not supported in Expo Go! Use EAS (`expo prebuild`) or eject to a bare workflow instead.'
      );
    } else {
      throw new Error('\n* Make sure you ran `expo prebuild`.');
    }
  }
}

export function verifyOnDevice(Module: any) {
  if (global.nativeCallSyncHook == null || Module.install == null) {
    throw new Error(
      'Failed to install react-native-audio-context: React Native is not running on-device. Oscillator can only be used when synchronous method invocations (JSI) are possible. If you are using a remote debugger (e.g. Chrome), switch to an on-device debugger (e.g. Flipper) instead.'
    );
  }
}

export function installModule(Module: any) {
  const result = Module.install();
  if (result !== true)
    throw new Error(
      `Failed to install react-native-audio-context: The native Oscillator Module could not be installed! Looks like something went wrong when installing JSI bindings: ${result}`
    );
}
