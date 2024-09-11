import type {
  OscillatorNode,
  GainNode,
  StereoPannerNode,
  BiquadFilterNode,
  AudioDestinationNode,
  AudioBufferSourceNode,
  AudioBuffer,
  ContextState,
} from '../types';

type AudioContext = {
  destination: AudioDestinationNode;
  state: ContextState;
  sampleRate: number;
  currentTime: number;
  createOscillator: () => OscillatorNode;
  createGain: () => GainNode;
  createStereoPanner: () => StereoPannerNode;
  createBiquadFilter: () => BiquadFilterNode;
  createBufferSource: () => AudioBufferSourceNode;
  createBuffer: (
    channels: number,
    length: number,
    sampleRate: number
  ) => AudioBuffer;
  close: () => void;
};

declare global {
  function nativeCallSyncHook(): unknown;
  var __AudioContext: AudioContext;
}
