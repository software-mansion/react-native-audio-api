import type {
  OscillatorNode,
  GainNode,
  StereoPannerNode,
  BiquadFilterNode,
  AudioDestinationNode,
  ContextState,
} from '../types';

type AudioContext = {
  destination: AudioDestinationNode | null;
  state: ContextState;
  sampleRate: number;
  currentTime: number;
  createOscillator: () => OscillatorNode;
  createGain: () => GainNode;
  createStereoPanner: () => StereoPannerNode;
  createBiquadFilter: () => BiquadFilterNode;
  close: () => void;
};

export declare global {
  function nativeCallSyncHook(): unknown;
  var __AudioContext: AudioContext;
}
