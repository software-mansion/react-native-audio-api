import { NativeModules } from 'react-native';
const { AudioContextModule } = NativeModules;
import type {
  BaseAudioContext,
  AudioDestinationNode,
  Gain,
  StereoPanner,
  Oscillator,
  ContextState,
} from './types';
import { installACModule } from './utils/install';

installACModule();

export class AudioContext implements BaseAudioContext {
  readonly destination: AudioDestinationNode;
  readonly state: ContextState;
  readonly sampleRate: number;

  constructor() {
    if (global.__AudioContext == null) {
      AudioContextModule.installAudioContext();
    }
    this.destination = global.__AudioContext.destination;
    this.state = global.__AudioContext.state;
    this.sampleRate = global.__AudioContext.sampleRate;
  }

  getCurrentTime(): number {
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
