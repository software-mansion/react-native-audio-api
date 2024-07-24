import type { Oscillator, Gain } from '../types';

type AudioContext = {
  createOscillator: () => Oscillator;
  destination: AudioDestinationNode | null;
  createGain: () => Gain;
};

export declare global {
  function nativeCallSyncHook(): unknown;
  var __AudioContext: AudioContext;
}
