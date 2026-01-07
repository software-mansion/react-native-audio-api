import type { ShareableWorkletCallback } from '../interfaces';

// Hint: Copied from react-native-worklets
// Doesn't really matter what is inside, just need a unique type
export interface WorkletRuntime {
  __hostObjectWorkletRuntime: never;
  readonly name: string;
}

export interface IWorkletsModule {
  makeShareableCloneRecursive: (
    workletCallback: ShareableWorkletCallback
  ) => ShareableWorkletCallback;

  createWorkletRuntime: (runtimeName: string) => WorkletRuntime;
}

export interface IAudioAPIModule {
  get workletsModule(): IWorkletsModule | null;
  get canUseWorklets(): boolean;
  get workletsVersion(): string;
  get areWorkletsAvailable(): boolean;
  get isWorkletsVersionSupported(): boolean;
  createAudioRuntime(): WorkletRuntime | null;
}
