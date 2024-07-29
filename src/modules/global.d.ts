import type { Oscillator, Gain } from '../types';

type AudioContext = {
  createOscillator: () => Oscillator;
  createGain: () => Gain;
  createStereoPanner: () => StereoPanner;
  destination: AudioDestinationNode | null;
  createGain: () => Gain;
};

export declare global {
  function nativeCallSyncHook(): unknown;
  var __AudioContext: AudioContext;
}
