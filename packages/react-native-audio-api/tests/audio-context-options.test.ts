jest.mock('react-native', () => ({
  TurboModuleRegistry: {
    get: jest.fn(() => ({
      getDevicePreferredSampleRate: jest.fn(() => 44100),
    })),
  },
}));

jest.mock('../src/core/BaseAudioContext', () => {
  return class BaseAudioContext {
    constructor(_context: unknown) {}
  };
});

import type { IAudioContext } from '../src/jsi-interfaces';

describe('AudioContext options', () => {
  const createAudioContext = globalThis.createAudioContext;

  afterEach(() => {
    globalThis.createAudioContext = createAudioContext;
  });

  it('forwards the Android voice communication output profile to native', () => {
    const nativeContext = {} as IAudioContext;
    globalThis.createAudioContext = jest.fn(() => nativeContext);
    const AudioContext = require('../src/core/AudioContext')
      .default as typeof import('../src/core/AudioContext').default;

    new AudioContext({
      sampleRate: 24000,
      androidOutputProfile: 'voiceCommunication',
    });

    expect(globalThis.createAudioContext).toHaveBeenCalledWith(
      24000,
      'voiceCommunication'
    );
  });
});
