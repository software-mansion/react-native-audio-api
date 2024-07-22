import { NativeModules, Platform } from 'react-native';

import type {
  AudioDestinationNode,
  BaseAudioContext,
  Oscillator,
} from './types';

export class AudioContext implements BaseAudioContext {
  destination: AudioDestinationNode | null;

  constructor() {
    this.destination = null;

    if (Platform.OS === 'android') {
      NativeModules.AudioContextModule.installAudioContext();
      this.destination = global.__AudioContext.destination;
    }
  }

  createOscillator(): Oscillator {
    return global.__AudioContext.createOscillator();
  }
}
