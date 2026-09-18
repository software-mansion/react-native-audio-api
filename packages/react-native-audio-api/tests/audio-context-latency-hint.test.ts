import AudioContext from '../src/core/AudioContext';
import WebAudioContext from '../src/web-core/AudioContext.web';
import type { IAudioEventEmitter } from '../src/jsi-interfaces';
import type { AudioContextLatencyCategory } from '../src/types';

jest.mock('react-native', () => ({
  Image: { resolveAssetSource: jest.fn() },
  Platform: { OS: 'ios' },
  TurboModuleRegistry: { get: jest.fn(() => ({ install: jest.fn() })) },
}));

jest.mock('../src/system', () => ({
  __esModule: true,
  default: { getDevicePreferredSampleRate: () => 48000 },
}));

jest.mock('../src/core/AudioListener', () => ({
  __esModule: true,
  default: class AudioListenerStub {},
}));

jest.mock('../src/web-core/AudioListener.web', () => ({
  __esModule: true,
  default: class WebAudioListenerStub {},
}));

jest.mock('../src/web-core/AudioDestinationNode.web', () => ({
  __esModule: true,
  default: class WebAudioDestinationNodeStub {},
}));

describe('AudioContext latencyHint on web', () => {
  const windowAudioContext = jest.fn();

  beforeEach(() => {
    windowAudioContext.mockReset();
    (globalThis as { window?: unknown }).window = {
      AudioContext: windowAudioContext,
    };
  });

  const optionsPassedToBrowser = (options?: {
    latencyHint?: AudioContextLatencyCategory;
  }) => {
    new WebAudioContext(options);
    return windowAudioContext.mock.calls[0][0];
  };

  it('forwards the category to the browser', () => {
    expect(optionsPassedToBrowser({ latencyHint: 'playback' }).latencyHint).toBe(
      'playback'
    );
  });

  it('forwards no category when the caller gave none', () => {
    expect(optionsPassedToBrowser().latencyHint).toBeUndefined();
  });
});

describe('AudioContext latencyHint', () => {
  const createAudioContext = jest.fn();

  beforeEach(() => {
    createAudioContext.mockReset();
    createAudioContext.mockReturnValue({
      destination: {},
      listener: {},
      sampleRate: 48000,
    });

    globalThis.createAudioContext =
      createAudioContext as unknown as typeof globalThis.createAudioContext;
    globalThis.AudioEventEmitter = {
      addAudioEventListener: () => 'subscription',
      removeAudioEventListener: () => undefined,
    } as unknown as IAudioEventEmitter;
  });

  const argsPassedToNative = (options?: {
    sampleRate?: number;
    latencyHint?: AudioContextLatencyCategory;
  }) => {
    new AudioContext(options);
    return createAudioContext.mock.calls[0];
  };

  it.each<AudioContextLatencyCategory>(['interactive', 'balanced', 'playback'])(
    'forwards the %s category',
    (latencyHint) => {
      expect(argsPassedToNative({ latencyHint })[1]).toBe(latencyHint);
    }
  );

  it('forwards no hint when the caller gave none', () => {
    expect(argsPassedToNative()[1]).toBeUndefined();
  });

  it('keeps the sample rate as the first argument alongside a hint', () => {
    const [sampleRate, latencyHint] = argsPassedToNative({
      sampleRate: 44100,
      latencyHint: 'balanced',
    });

    expect(sampleRate).toBe(44100);
    expect(latencyHint).toBe('balanced');
  });
});
