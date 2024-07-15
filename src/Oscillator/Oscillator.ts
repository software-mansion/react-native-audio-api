import { NativeModules, Platform } from 'react-native';
import type { OscillatorType } from './types';

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

function getOscillator() {
  const OscillatorModule = NativeModules.Oscillator;
  if (OscillatorModule == null) {
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
  return OscillatorModule;
}

function verifyOnDevice(OscillatorModule: any) {
  if (global.nativeCallSyncHook == null || OscillatorModule.install == null) {
    throw new Error(
      'Failed to install react-native-audio-context: React Native is not running on-device. Oscillator can only be used when synchronous method invocations (JSI) are possible. If you are using a remote debugger (e.g. Chrome), switch to an on-device debugger (e.g. Flipper) instead.'
    );
  }
}

function installModule(OscillatorModule: any) {
  const result = OscillatorModule.install();
  if (result !== true)
    throw new Error(
      `Failed to install react-native-audio-context: The native Oscillator Module could not be installed! Looks like something went wrong when installing JSI bindings: ${result}`
    );
}

function verifyInstallation() {
  if (global.__OscillatorProxy == null)
    throw new Error(
      'Failed to install react-native-audio-context, the native initializer function does not exist. Are you trying to use Oscillator from different JS Runtimes?'
    );
}

function createOscillatorProxy(): () => OscillatorType {
  if (global.__OscillatorProxy) {
    return global.__OscillatorProxy;
  }

  verifyExpoGo();

  const OscillatorModule = getOscillator();

  verifyOnDevice(OscillatorModule);
  installModule(OscillatorModule);
  verifyInstallation();

  if (global.__OscillatorProxy == null) {
    throw new Error('Failed to initialize __OscillatorProxy.');
  }

  return global.__OscillatorProxy;
}

// Call the creator and export what it returns
export default createOscillatorProxy();
