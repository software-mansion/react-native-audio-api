// Import react-native-audio-api for its side effect: this guarantees the core
// native module has installed its JSI globals (createAudioContext, ...) before
// we install the worklet extensions on top of them.
import 'react-native-audio-api';
import semverGte from 'semver/functions/gte';

import NativeAudioWorkletsModule from './specs/NativeAudioWorkletsModule';
import type { IWorkletsModule } from './types';

const MIN_WORKLETS_VERSION = '0.10.0';

class AudioWorkletsModuleImpl {
  #workletsModule: IWorkletsModule | null = null;
  #workletsVersion = 'unknown';

  constructor() {
    this.#verifyWorklets();

    if (!NativeAudioWorkletsModule) {
      throw new Error(
        'react-native-audio-worklets: native module not found. This package requires React Native ' +
          'New Architecture (TurboModules). Enable newArchEnabled on Android and ' +
          'RCT_NEW_ARCH_ENABLED=1 in your Podfile, then rebuild the native app.'
      );
    }

    if (!this.#isInstalled()) {
      NativeAudioWorkletsModule.install();
    }
  }

  #verifyWorklets(): void {
    let workletsPackage: IWorkletsModule;
    try {
      workletsPackage = require('react-native-worklets');
      this.#workletsVersion =
        require('react-native-worklets/package.json').version;
    } catch {
      throw new Error(
        'react-native-audio-worklets requires react-native-worklets to be installed.'
      );
    }

    if (!semverGte(this.#workletsVersion, MIN_WORKLETS_VERSION)) {
      throw new Error(
        `react-native-audio-worklets requires react-native-worklets >= ${MIN_WORKLETS_VERSION}, ` +
          `but ${this.#workletsVersion} is installed.`
      );
    }

    this.#workletsModule = workletsPackage;
  }

  #isInstalled(): boolean {
    return globalThis.__createWorkletNode != null;
  }

  get workletsModule(): IWorkletsModule {
    return this.#workletsModule!;
  }
}

const AudioWorkletsModule = new AudioWorkletsModuleImpl();
export default AudioWorkletsModule;
