import type { Oscillator } from '../types';

type AudioContextProxy = {
  createOscillator: () => Oscillator;
  destination(): AudioDestinationNode;
};

export declare global {
  function nativeCallSyncHook(): unknown;
  var __AudioContextProxy: AudioContextProxy;
}
