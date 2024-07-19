import { NativeModules, Platform } from 'react-native';

import type {
  AudioDestinationNode,
  BaseAudioContext,
  Oscillator,
} from './types';

export class AudioContext implements BaseAudioContext {
  constructor() {
    if (Platform.OS === 'android') {
      NativeModules.AudioContextModule.initAudioContext();
    }
  }

  public createOscillator(): Oscillator {
    return global.__AudioContextProxy.createOscillator();
  }

  public destination(): AudioDestinationNode {
    return global.__AudioContextProxy.destination();
  }
}
