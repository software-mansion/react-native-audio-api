export function clamp(value: number, min: number, max: number): number {
  return Math.min(Math.max(value, min), max);
}

export let isWorkletsAvailable = false;

// eslint-disable-next-line @typescript-eslint/no-explicit-any
export let workletsModule: any = null;

try {
  workletsModule = require('react-native-worklets');
  isWorkletsAvailable = true;
} catch (error) {
  isWorkletsAvailable = false;
}
