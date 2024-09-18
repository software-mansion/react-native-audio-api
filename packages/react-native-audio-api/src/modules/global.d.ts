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

export type AudioContextType = {
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

type AudioAPI = {
  createAudioContext: () => AudioContext_;
};

declare global {
  function nativeCallSyncHook(): unknown;
  var __AudioAPI: AudioAPI;
}
