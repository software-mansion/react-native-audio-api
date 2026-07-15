// Import react-native-audio-api for its side effect: this guarantees the core
// native module has installed its JSI globals (createAudioContext, ...) before
// we install the worklet extensions on top of them.
import { NotSupportedError } from 'react-native-audio-api';
import semverGte from 'semver/functions/gte';
import type { WorkletRuntime } from 'react-native-worklets';

import NativeAudioWorkletsModule from './specs/NativeAudioWorkletsModule';

const MIN_WORKLETS_VERSION = '0.10.0';

type WorkletsModule = typeof import('react-native-worklets');

class AudioWorkletsModuleImpl {
  #workletsModule: WorkletsModule | null = null;
  #workletsVersion = 'unknown';
  #audioRuntime?: WorkletRuntime;

  constructor() {
    this.#verifyWorklets();

    if (!NativeAudioWorkletsModule) {
      throw new NotSupportedError(
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
    let workletsPackage: WorkletsModule;
    try {
      workletsPackage = require('react-native-worklets');
      this.#workletsVersion =
        require('react-native-worklets/package.json').version;
    } catch {
      throw new NotSupportedError(
        'react-native-audio-worklets requires react-native-worklets to be installed.'
      );
    }

    if (!semverGte(this.#workletsVersion, MIN_WORKLETS_VERSION)) {
      throw new NotSupportedError(
        `react-native-audio-worklets requires react-native-worklets >= ${MIN_WORKLETS_VERSION}, ` +
          `but ${this.#workletsVersion} is installed.`
      );
    }

    this.#workletsModule = workletsPackage;
  }

  #isInstalled(): boolean {
    return (
      globalThis.__createWorkletNode != null &&
      globalThis.__createWorkletSourceNode != null &&
      globalThis.__createWorkletProcessingNode != null
    );
  }

  get workletsModule(): WorkletsModule {
    return this.#workletsModule!;
  }

  getAudioRuntime(): WorkletRuntime {
    if (this.#audioRuntime === undefined) {
      this.#audioRuntime = this.#workletsModule!.createWorkletRuntime({
        name: 'AudioWorkletRuntime',
        enableEventLoop: false,
        queue: null,
      });
    }

    return this.#audioRuntime;
  }
}

const AudioWorkletsModule = new AudioWorkletsModuleImpl();
export default AudioWorkletsModule;
