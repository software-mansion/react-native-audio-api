import { NativeModules, Platform } from 'react-native';
const { AudioContextModule } = NativeModules;
import type {
  Oscillator,
  BaseAudioContext,
  AudioDestinationNode,
  Gain,
  StereoPanner,
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
    this.state = global.__AudioContext.state;
    this.sampleRate = global.__AudioContext.sampleRate;

    if (Platform.OS === 'android') {
      AudioContextModule.installAudioContext();
      this.destination = global.__AudioContext.destination;
    }
  }

  public get currentTime() {
    return global.__AudioContext.currentTime;
  }

  createOscillator(): Oscillator {
    return global.__AudioContext.createOscillator();
  }

  createGain(): Gain {
    return global.__AudioContext.createGain();
  }

  createStereoPanner(): StereoPanner {
    return global.__AudioContext.createStereoPanner();
  }
}

export type { Oscillator, Gain, StereoPanner };
