import type { ShareableWorkletCallback } from '../interfaces';
import { NativeAudioAPIModule } from '../specs';

// Hint: Copied from react-native-worklets
// Doesn't really matter what is inside, just need a unique type
interface WorkletRuntime {
  __hostObjectWorkletRuntime: never;
  readonly name: string;
}

interface IWorkletsModule {
  makeShareableCloneRecursive: (
    workletCallback: ShareableWorkletCallback
  ) => ShareableWorkletCallback;

  createWorkletRuntime: (runtimeName: string) => WorkletRuntime;
}

class AudioAPIModule {
  #workletsModule_: IWorkletsModule | null = null;
  #canUseWorklets_ = false;
  #workletsVersion = 'unknown';
  #workletsAvailable_ = false;
  #audioRuntime: WorkletRuntime | null = null;

  public supportedWorkletsVersion = ['0.6.0', '0.6.1'];

  constructor() {
    // Important! Verify and import worklets first
    // otherwise the native module installation might crash
    // if react-native-worklets is not imported before audio-api
    this.#verifyWorklets();

    if (this.#verifyInstallation()) {
      return;
    }

    if (!NativeAudioAPIModule) {
      throw new Error(
        `Failed to install react-native-audio-api: The native module could not be found.`
      );
    }

    NativeAudioAPIModule.install();
  }

  #verifyWorklets(): boolean {
    try {
      const workletsPackage = require('react-native-worklets');
      const workletsPackageJson = require('react-native-worklets/package.json');
      this.#workletsVersion = workletsPackageJson.version;
      this.#workletsAvailable_ = true;

      this.#canUseWorklets_ = this.supportedWorkletsVersion.includes(
        workletsPackageJson.version
      );

      if (this.#canUseWorklets_) {
        this.#workletsModule_ = workletsPackage;
      }

      return this.#canUseWorklets_;
    } catch {
      this.#canUseWorklets_ = false;
      return false;
    }
  }

  #verifyInstallation(): boolean {
    return (
      global.createAudioContext != null &&
      global.createOfflineAudioContext != null &&
      global.createAudioRecorder != null &&
      global.createAudioDecoder != null &&
      global.createAudioStretcher != null &&
      global.AudioEventEmitter != null
    );
  }

  get workletsModule(): IWorkletsModule | null {
    return this.#workletsModule_;
  }

  /**
   * Indicates whether react-native-worklets are installed in matching version,
   * for usage with react-native-audio-api.
   */
  get canUseWorklets(): boolean {
    return this.#canUseWorklets_;
  }

  /** Returns the installed worklets version or 'unknown'. */
  get workletsVersion(): string {
    return this.#workletsVersion;
  }

  /**
   * Indicates whether react-native-worklets are installed, regardless of
   * version support. Useful for asserting compatibility.
   */
  get areWorkletsAvailable(): boolean {
    return this.#workletsAvailable_;
  }

  /**
   * Indicates whether the installed react-native-worklets version is supported.
   * Useful for asserting compatibility.
   */
  get isWorkletsVersionSupported(): boolean {
    // Note: if areWorkletsAvailable is true, canUseWorklets is equivalent to version check
    return this.#canUseWorklets_;
  }

  public getOrCreateAudioRuntime(): WorkletRuntime | null {
    if (!this.#canUseWorklets_) {
      return null;
    }

    if (this.#audioRuntime) {
      return this.#audioRuntime;
    }

    this.#audioRuntime = this.#workletsModule_!.createWorkletRuntime(
      'AudioWorkletRuntime'
    );

    return this.#audioRuntime;
  }
}

export default new AudioAPIModule();
