import type {
  Oscillator,
  Gain,
  StereoPanner,
  AudioDestinationNode,
  ContextState,
} from '../types';

type AudioContext = {
  createOscillator: () => Oscillator;
  createGain: () => Gain;
  createStereoPanner: () => StereoPanner;
  destination: AudioDestinationNode;
  state: ContextState;
  sampleRate: number;
  currentTime: number;
};

export declare global {
  function nativeCallSyncHook(): unknown;
  var __AudioContext: AudioContext;
}
