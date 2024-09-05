import { NativeModules, Platform } from 'react-native';

export function installACModule() {
  verifyExpoGo();

  const AudioAPIModule = getAudioAPIModule();

  verifyOnDevice(AudioAPIModule);
  installModule(AudioAPIModule);

  verifyInstallation();
}

function getAudioAPIModule() {
  const AudioAPIModule = NativeModules.AudioAPIModule;

  if (AudioAPIModule == null) {
    let message =
      'Failed to install react-native-audio-api: The native `AudioContext` Module could not be found.';
    message +=
      '\n* Make sure react-native-audio-api is correctly autolinked (run `npx react-native config` to verify)';
    if (Platform.OS === 'ios' || Platform.OS === 'macos') {
      message += '\n* Make sure you ran `pod install` in the ios/ directory.';
    }
    if (Platform.OS === 'android') {
      message += '\n* Make sure gradle is synced.';
    }
    message += '\n* Make sure you rebuilt the app.';
    throw new Error(message);
  }

  return AudioAPIModule;
}

function verifyExpoGo() {
  const ExpoConstants =
    NativeModules.NativeUnimoduleProxy?.modulesConstants?.ExponentConstants;
  if (ExpoConstants != null) {
    if (ExpoConstants.appOwnership === 'expo') {
      throw new Error(
        'react-native-audio-api is not supported in Expo Go! Use EAS (`expo prebuild`) or eject to a bare workflow instead.'
      );
    } else {
      throw new Error('\n* Make sure you ran `expo prebuild`.');
    }
  }
}

function verifyOnDevice(Module: any) {
  if (global.nativeCallSyncHook == null || Module.install == null) {
    throw new Error(
      'Failed to install react-native-audio-api: React Native is not running on-device. Audio Context can only be used when synchronous method invocations (JSI) are possible. If you are using a remote debugger (e.g. Chrome), switch to an on-device debugger (e.g. Flipper) instead.'
    );
  }
}

function installModule(Module: any) {
  const result = Module.install();
  if (result !== true) {
    throw new Error(
      `Failed to install react-native-audio-api: The native Audio Context Module could not be installed! Looks like something went wrong when installing JSI bindings: ${result}`
    );
  }
}

function verifyInstallation() {
  if (global.__AudioContext == null) {
    throw new Error(
      'Failed to install react-native-audio-api, the native initializer private does not exist. Are you trying to use Audio Context from different JS Runtimes?'
    );
  }
}
