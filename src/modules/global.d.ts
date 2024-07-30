import type { Oscillator } from '../types';

type AudioContext = {
  createOscillator: () => Oscillator;
  createGain: () => Gain;
  createStereoPanner: () => StereoPanner;
  destination: AudioDestinationNode | null;
};

export declare global {
  function nativeCallSyncHook(): unknown;
  var __AudioContext: AudioContext;
}
