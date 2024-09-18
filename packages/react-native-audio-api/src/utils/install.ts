import { NativeModules, Platform } from 'react-native';

export function installModule() {
  const AudioAPIModule = NativeModules.AudioAPIModule;

  if (AudioAPIModule == null) {
    throw new Error(buildErrorMessage());
  }

  verifyOnDevice(AudioAPIModule);
  runInstall(AudioAPIModule);
  verifyInstallation();

  return AudioAPIModule;
}

function buildErrorMessage(): string {
  let message = `
    Failed to install react-native-audio-api: The native 'AudioAPI' Module could not be found.

    * Make sure react-native-audio-api is correctly autolinked (run 'npx react-native config' to verify).
  `;

  message += verifyAppleOS();
  message += verifyExpo();

  message += '\n* Make sure you rebuilt the app.';

  return message.trim();
}

function verifyAppleOS(): string {
  if (Platform.OS === 'ios' || Platform.OS === 'macos') {
    return '\n* Make sure you ran `pod install` in the ios/ directory.';
  }

  return '';
}

function verifyExpo(): string {
  const ExpoConstants =
    NativeModules.NativeUnimoduleProxy?.modulesConstants?.ExponentConstants;

  if (!ExpoConstants) {
    return '';
  }

  if (ExpoConstants.appOwnership === 'expo') {
    return "\n* 'react-native-audio-api' is not supported in Expo Go! Use EAS (`expo prebuild`) or eject to a bare workflow instead.";
  }

  return "\n* Make sure you ran 'expo prebuild'.";
}

function verifyOnDevice(Module: any) {
  if (global.nativeCallSyncHook == null || Module.install == null) {
    throw new Error(
      'Failed to install react-native-audio-api: React Native is not running on-device. Audio API can only be used when synchronous method invocations (JSI) are possible. If you are using a remote debugger (e.g. Chrome), switch to an on-device debugger (e.g. Flipper) instead.'
    );
  }
}

function runInstall(Module: any) {
  const result = Module.install();

  if (result !== true) {
    throw new Error(
      `Failed to install react-native-audio-api: The native Audio API Module could not be installed! Looks like something went wrong when installing JSI bindings: ${result}`
    );
  }
}

function verifyInstallation() {
  if (global.__AudioAPI == null) {
    throw new Error(
      'Failed to install react-native-audio-api, the native initializer private does not exist. Are you trying to use Audio API from different JS Runtimes?'
    );
  }
}
