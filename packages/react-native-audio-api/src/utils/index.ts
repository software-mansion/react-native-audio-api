import type { ShareableWorkletCallback } from '../interfaces';

interface SimplifiedWorkletModule {
  makeShareableCloneRecursive: (
    workletCallback: ShareableWorkletCallback
  ) => ShareableWorkletCallback;
}

export function clamp(value: number, min: number, max: number): number {
  return Math.min(Math.max(value, min), max);
}

export let isWorkletsAvailable = false;
export let workletsModule: SimplifiedWorkletModule;

try {
  workletsModule = require('react-native-worklets');
  isWorkletsAvailable = true;
} catch (error) {
  isWorkletsAvailable = false;
}
