import { NativeModules, Platform } from 'react-native';
const { AudioContextModule } = NativeModules;
import type {
  BaseAudioContext,
  AudioDestinationNode,
  GainNode,
  StereoPannerNode,
  OscillatorNode,
  ContextState,
} from './types';
import { installACModule } from './utils/install';

installACModule();

export class AudioContext implements BaseAudioContext {
  readonly destination: AudioDestinationNode | null;
  readonly state: ContextState;
  readonly sampleRate: number;

  constructor() {
    this.destination = null;

    if (Platform.OS === 'android') {
      AudioContextModule.installAudioContext();
      this.destination = global.__AudioContext.destination;
    }

    this.state = global.__AudioContext.state;
    this.sampleRate = global.__AudioContext.sampleRate;
  }

  public get currentTime() {
    return global.__AudioContext.currentTime;
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

  close(): void {
    global.__AudioContext.close();
  }
}

export type { GainNode, StereoPannerNode, OscillatorNode };
