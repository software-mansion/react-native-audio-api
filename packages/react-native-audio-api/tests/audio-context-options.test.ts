import { NotSupportedError } from '../src/errors';
import type {
  IAudioContext,
  IOfflineAudioContext,
} from '../src/jsi-interfaces';

let mockDeviceBufferDuration = 0.01;

jest.mock('react-native', () => ({
  TurboModuleRegistry: {
    get: jest.fn(() => ({
      getDevicePreferredSampleRate: jest.fn(() => 44100),
      getDeviceBufferDuration: jest.fn(() => mockDeviceBufferDuration),
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
      'voiceCommunication',
      128
    );
  });
});

describe('AudioContext renderSizeHint', () => {
  const createAudioContext = globalThis.createAudioContext;

  const renderQuantumSizeFor = (
    options: import('../src/types').AudioContextOptions
  ): number => {
    globalThis.createAudioContext = jest.fn(() => ({}) as IAudioContext);
    const AudioContext = require('../src/core/AudioContext')
      .default as typeof import('../src/core/AudioContext').default;
    expect(new AudioContext(options)).toBeInstanceOf(AudioContext);
    return (globalThis.createAudioContext as jest.Mock).mock.calls[0][3];
  };

  afterEach(() => {
    globalThis.createAudioContext = createAudioContext;
    mockDeviceBufferDuration = 0.01;
  });

  it('uses 128 frames by default', () => {
    expect(renderQuantumSizeFor({ sampleRate: 48000 })).toBe(128);
    expect(
      renderQuantumSizeFor({ sampleRate: 48000, renderSizeHint: 'default' })
    ).toBe(128);
  });

  it('passes a non-power-of-two size through unchanged', () => {
    expect(
      renderQuantumSizeFor({ sampleRate: 44100, renderSizeHint: 441 })
    ).toBe(441);
  });

  it('accepts up to six seconds of frames and rejects anything outside', () => {
    expect(
      renderQuantumSizeFor({ sampleRate: 3000, renderSizeHint: 18000 })
    ).toBe(18000);
    expect(() =>
      renderQuantumSizeFor({ sampleRate: 3000, renderSizeHint: 18001 })
    ).toThrow(NotSupportedError);
    expect(() =>
      renderQuantumSizeFor({ sampleRate: 48000, renderSizeHint: 0 })
    ).toThrow(NotSupportedError);
    expect(() =>
      renderQuantumSizeFor({ sampleRate: 48000, renderSizeHint: 100.5 })
    ).toThrow(NotSupportedError);
  });

  it('resolves hardware to the device callback length at the context rate', () => {
    mockDeviceBufferDuration = 0.01;
    expect(
      renderQuantumSizeFor({ sampleRate: 24000, renderSizeHint: 'hardware' })
    ).toBe(240);
  });

  it('falls back to 128 frames when the device reports no callback length', () => {
    mockDeviceBufferDuration = 0;
    expect(
      renderQuantumSizeFor({ sampleRate: 48000, renderSizeHint: 'hardware' })
    ).toBe(128);
  });
});

describe('OfflineAudioContext renderSizeHint', () => {
  const createOfflineAudioContext = globalThis.createOfflineAudioContext;

  afterEach(() => {
    globalThis.createOfflineAudioContext = createOfflineAudioContext;
  });

  it('resolves hardware to 128 frames, as the spec requires offline', () => {
    globalThis.createOfflineAudioContext = jest.fn(
      () => ({}) as IOfflineAudioContext
    );
    const OfflineAudioContext = require('../src/core/OfflineAudioContext')
      .default as typeof import('../src/core/OfflineAudioContext').default;

    expect(
      new OfflineAudioContext({
        numberOfChannels: 1,
        length: 44100,
        sampleRate: 44100,
        renderSizeHint: 'hardware',
      })
    ).toBeInstanceOf(OfflineAudioContext);

    expect(globalThis.createOfflineAudioContext).toHaveBeenCalledWith(
      1,
      44100,
      44100,
      128
    );
  });

  it('returns the native output timestamp unchanged', () => {
    const timestamp = { contextTime: 1.25, performanceTime: 98765.4 };
    const nativeContext = {
      getOutputTimestamp: jest.fn(() => timestamp),
    } as unknown as IAudioContext;
    globalThis.createAudioContext = jest.fn(() => nativeContext);
    const AudioContext = require('../src/core/AudioContext')
      .default as typeof import('../src/core/AudioContext').default;

    expect(new AudioContext().getOutputTimestamp()).toEqual(timestamp);
  });
});
