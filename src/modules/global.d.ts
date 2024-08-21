import type {
  Oscillator,
  Gain,
  StereoPanner,
  AudioDestinationNode,
  ContextState,
} from '../types';

type AudioContext = {
  destination: AudioDestinationNode;
  state: ContextState;
  sampleRate: number;
  currentTime: number;
  createOscillator: () => Oscillator;
  createGain: () => Gain;
  createStereoPanner: () => StereoPanner;
  close: () => void;
};

export declare global {
  function nativeCallSyncHook(): unknown;
  var __AudioContext: AudioContext;
}
