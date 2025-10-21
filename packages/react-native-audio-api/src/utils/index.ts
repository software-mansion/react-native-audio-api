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

export let isWorkletsAvailable = false;
export let workletsModule: SimplifiedWorkletModule;

try {
  workletsModule = require('react-native-worklets');
  isWorkletsAvailable = true;
  // eslint-disable-next-line @typescript-eslint/no-unused-vars
} catch (error) {
  isWorkletsAvailable = false;
}
