import type { Oscillator } from '../types';

type AudioContextProxy = {
  createOscillator: (frequency: number) => Oscillator;
};

export declare global {
  function nativeCallSyncHook(): unknown;
  var __AudioContextProxy: AudioContextProxy;
}
