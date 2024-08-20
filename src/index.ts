import { Platform } from 'react-native';
import type {
  BaseAudioContext,
  AudioDestinationNode,
  GainNode,
  StereoPannerNode,
  OscillatorNode,
  BiquadFilterNode,
} from './types';
import { installACModule } from './utils/install';

export class AudioContext implements BaseAudioContext {
  readonly destination: AudioDestinationNode | null;
  readonly sampleRate: number;

  constructor() {
    this.destination = null;

    if (global.__AudioContext == null) {
      installACModule();
    }

    if (Platform.OS === 'android') {
      this.destination = global.__AudioContext.destination;
    }
    this.sampleRate = global.__AudioContext.sampleRate;
  }

  public get currentTime() {
    return global.__AudioContext.currentTime;
  }

  public get state() {
    return global.__AudioContext.state;
  }

  createOscillator(): OscillatorNode {
    return global.__AudioContext.createOscillator();
  }

  createGain(): GainNode {
    return global.__AudioContext.createGain();
  }

  createStereoPanner(): StereoPannerNode {
    return global.__AudioContext.createStereoPanner();
  }

  createBiquadFilter(): BiquadFilterNode {
    return global.__AudioContext.createBiquadFilter();
  }

  close(): void {
    if (Platform.OS === 'android') {
      global.__AudioContext.close();
    }
  }
}

export type { GainNode, StereoPannerNode, OscillatorNode, BiquadFilterNode };
