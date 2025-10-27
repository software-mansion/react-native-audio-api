import type { ShareableWorkletCallback } from '../interfaces';

interface SimplifiedWorkletModule {
  makeShareableCloneRecursive: (
    workletCallback: ShareableWorkletCallback
  ) => ShareableWorkletCallback;

  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  createWorkletRuntime: (options?: any) => any;
}

export function clamp(value: number, min: number, max: number): number {
  return Math.min(Math.max(value, min), max);
}

let isWorkletsAvailable = false;
export let isWorkletsVersionSupported = false;
export let workletsVersion = 'unknown';
export const supportedWorkletsVersions = ['0.6.0', '0.6.1'];
export let workletsModule: SimplifiedWorkletModule;

export function assertWorkletsEnabled() {
  if (!isWorkletsAvailable) {
    throw new Error(
      '[react-native-audio-api]: Worklets are not available. Please install react-native-worklets to use this feature.'
    );
  } else if (!isWorkletsVersionSupported) {
    throw new Error(
      `[react-native-audio-api]: Worklets version ${workletsVersion} is not supported.
      Please install react-native-worklets of one of the following versions: [${supportedWorkletsVersions.join(', ')}] to use this feature.`
    );
  }
}

try {
  workletsModule = require('react-native-worklets');
  const workletsModuleJson = require('react-native-worklets/package.json');
  isWorkletsVersionSupported = supportedWorkletsVersions.includes(
    workletsModuleJson.version
  );
  isWorkletsAvailable = true;
  workletsVersion = workletsModuleJson.version;
  // eslint-disable-next-line @typescript-eslint/no-unused-vars
} catch (error) {
  isWorkletsAvailable = false;
}
