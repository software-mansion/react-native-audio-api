import { NativeModules, Platform } from 'react-native';
import type { JSIExampleWrapper } from './types';

function verifyExpoGo() {
  const ExpoConstants =
    NativeModules.NativeUnimoduleProxy?.modulesConstants?.ExponentConstants;
  if (ExpoConstants != null) {
    if (ExpoConstants.appOwnership === 'expo') {
      throw new Error(
        'react-native-fast-crypto is not supported in Expo Go! Use EAS (`expo prebuild`) or eject to a bare workflow instead.'
      );
    } else {
      throw new Error('\n* Make sure you ran `expo prebuild`.');
    }
  }
}

function getJSIExample() {
  const JSIExampleModule = NativeModules.JSIExample;
  if (JSIExampleModule == null) {
    let message =
      'Failed to install react-native-fast-crypto: The native `JSIExample` Module could not be found.';
    message +=
      '\n* Make sure react-native-fast-crypto is correctly autolinked (run `npx react-native config` to verify)';
    if (Platform.OS === 'ios' || Platform.OS === 'macos') {
      message += '\n* Make sure you ran `pod install` in the ios/ directory.';
    }
    if (Platform.OS === 'android') {
      message += '\n* Make sure gradle is synced.';
    }
    message += '\n* Make sure you rebuilt the app.';
    throw new Error(message);
  }
  return JSIExampleModule;
}

function verifyOnDevice(JSIExampleModule: any) {
  if (global.nativeCallSyncHook == null || JSIExampleModule.install == null) {
    throw new Error(
      'Failed to install react-native-fast-crypto: React Native is not running on-device. JSIExample can only be used when synchronous method invocations (JSI) are possible. If you are using a remote debugger (e.g. Chrome), switch to an on-device debugger (e.g. Flipper) instead.'
    );
  }
}

function installModule(JSIExampleModule: any) {
  const result = JSIExampleModule.install();
  if (result !== true)
    throw new Error(
      `Failed to install react-native-fast-crypto: The native JSIExample Module could not be installed! Looks like something went wrong when installing JSI bindings: ${result}`
    );
}

function verifyInstallation() {
  if (global.__JSIExampleProxy == null)
    throw new Error(
      'Failed to install react-native-fast-crypto, the native initializer function does not exist. Are you trying to use JSIExample from different JS Runtimes?'
    );
}

function createJSIExampleProxy(): JSIExampleWrapper {
  if (global.__JSIExampleProxy) {
    return global.__JSIExampleProxy;
  }

  verifyExpoGo();

  const JSIExampleModule = getJSIExample();

  verifyOnDevice(JSIExampleModule);
  installModule(JSIExampleModule);
  verifyInstallation();

  if (global.__JSIExampleProxy == null) {
    throw new Error('Failed to initialize __JSIExampleProxy.');
  }

  return global.__JSIExampleProxy;
}

// Call the creator and export what it returns
export default createJSIExampleProxy();
