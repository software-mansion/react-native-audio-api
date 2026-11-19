import type { IAudioContext } from '../src/jsi-interfaces';

jest.mock('react-native', () => ({
  TurboModuleRegistry: {
    get: jest.fn(() => ({
      getDevicePreferredSampleRate: jest.fn(() => 44100),
    })),
  },
}));

jest.mock('../src/core/BaseAudioContext', () => {
  return class BaseAudioContext {
    context: unknown;
    audioEventEmitter = {
      addAudioEventListener: () => ({ subscriptionId: '0' }),
    };

    constructor(context: unknown) {
      this.context = context;
    }
  };
});

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

    expect(
      new AudioContext({
        sampleRate: 24000,
        androidOutputProfile: 'voiceCommunication',
      })
    ).toBeInstanceOf(AudioContext);

    expect(globalThis.createAudioContext).toHaveBeenCalledWith(
      24000,
      undefined,
      'voiceCommunication'
    );
  });
});
