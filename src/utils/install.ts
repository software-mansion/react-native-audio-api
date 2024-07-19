import { NativeModules, Platform } from 'react-native';

export function installACModule() {
  if (Platform.OS !== 'ios') {
    return;
  }

  verifyExpoGo();

  const AudioContextModule = getAudioContextModule();

  verifyOnDevice(AudioContextModule);
  installModule(AudioContextModule);
  verifyInstallation();
}

function getAudioContextModule() {
  const AudioContextModule = NativeModules.AudioContextModule;
  if (AudioContextModule == null) {
    let message =
      'Failed to install react-native-audio-context: The native `AudioContext` Module could not be found.';
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

function verifyExpoGo() {
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

function verifyOnDevice(Module: any) {
  if (global.nativeCallSyncHook == null || Module.install == null) {
    throw new Error(
      'Failed to install react-native-audio-context: React Native is not running on-device. Audio Context can only be used when synchronous method invocations (JSI) are possible. If you are using a remote debugger (e.g. Chrome), switch to an on-device debugger (e.g. Flipper) instead.'
    );
  }
}

function installModule(Module: any) {
  const result = Module.install();
  if (result !== true) {
    throw new Error(
      `Failed to install react-native-audio-context: The native Audio Context Module could not be installed! Looks like something went wrong when installing JSI bindings: ${result}`
    );
  }
}

function verifyInstallation() {
  if (global.__AudioContextProxy == null) {
    throw new Error(
      'Failed to install react-native-audio-context, the native initializer private does not exist. Are you trying to use Audio Context from different JS Runtimes?'
    );
  }
}
