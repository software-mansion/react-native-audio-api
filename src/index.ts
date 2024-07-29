import { NativeModules, Platform } from 'react-native';
const { AudioContextModule } = NativeModules;
import type {
  Oscillator,
  BaseAudioContext,
  AudioDestinationNode,
  Gain,
  StereoPanner,
} from './types';
import { installACModule } from './utils/install';

installACModule();

export class AudioContext implements BaseAudioContext {
  destination: AudioDestinationNode | null;

  constructor() {
    this.destination = null;

    if (Platform.OS === 'android') {
      AudioContextModule.installAudioContext();
      this.destination = global.__AudioContext.destination;
    }
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
