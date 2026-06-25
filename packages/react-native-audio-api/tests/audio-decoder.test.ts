/* eslint-disable @typescript-eslint/no-var-requires */

import type { IBaseAudioContext, IAudioDecoder } from '../src/jsi-interfaces';

type AudioDecoderExports = typeof import('../src/core/AudioDecoder');

jest.mock('react-native', () => ({
  Image: {
    resolveAssetSource: jest.fn((input: number) => ({
      uri: `file:///asset-${input}.wav`,
    })),
  },
  Platform: {
    OS: 'ios',
  },
  TurboModuleRegistry: {
    get: jest.fn(() => ({
      install: jest.fn(),
      readAndroidReleaseAssetBytesAsBase64: jest.fn(),
    })),
  },
}));

const createNativeAudioNode = () => ({
  numberOfInputs: 0,
  numberOfOutputs: 1,
  channelCount: 2,
  channelCountMode: 'explicit',
  channelInterpretation: 'speakers',
  connect: jest.fn(),
  disconnect: jest.fn(),
});

const createDecoder = (duration: number = 12.5) =>
  ({
    decodeWithMemoryBlock: jest.fn(),
    decodeWithFilePath: jest.fn(),
    getDurationWithFilePath: jest.fn().mockResolvedValue(duration),
    decodeWithPCMInBase64: jest.fn(),
  }) as unknown as jest.Mocked<IAudioDecoder>;

const installDecoder = (decoder: IAudioDecoder) => {
  globalThis.createAudioDecoder = jest.fn(() => decoder);
};

const loadAudioDecoder = (): AudioDecoderExports => {
  return require('../src/core/AudioDecoder') as AudioDecoderExports;
};

describe('getAudioDuration', () => {
  beforeEach(() => {
    jest.resetModules();
  });

  it('routes local file paths to the native duration decoder', async () => {
    const decoder = createDecoder();
    installDecoder(decoder);

    const { getAudioDuration } = loadAudioDecoder();

    await expect(
      getAudioDuration('file:///tmp/audio%20file.wav')
    ).resolves.toBe(12.5);
    expect(decoder.getDurationWithFilePath).toHaveBeenCalledWith(
      '/tmp/audio file.wav'
    );
    expect(decoder.decodeWithFilePath).not.toHaveBeenCalled();
  });

  it('rejects ArrayBuffer input without decoding audio data', async () => {
    const decoder = createDecoder();
    installDecoder(decoder);

    const { getAudioDuration } = loadAudioDecoder();

    await expect(
      getAudioDuration(new ArrayBuffer(8) as unknown as string)
    ).rejects.toThrow(
      'ArrayBuffer duration probing is not currently supported.'
    );
    expect(decoder.getDurationWithFilePath).not.toHaveBeenCalled();
  });

  it('rejects asset module ids without resolving bundled assets', async () => {
    const decoder = createDecoder();
    installDecoder(decoder);

    const { getAudioDuration } = loadAudioDecoder();

    await expect(getAudioDuration(1 as unknown as string)).rejects.toThrow(
      'Input must be a local file path or file:// URI.'
    );
    expect(decoder.getDurationWithFilePath).not.toHaveBeenCalled();
  });

  it('rejects remote URL input without fetching or decoding', async () => {
    const decoder = createDecoder();
    installDecoder(decoder);

    const { getAudioDuration } = loadAudioDecoder();

    await expect(
      getAudioDuration('https://example.com/audio.mp3')
    ).rejects.toThrow(
      'Remote source duration probing is not currently supported.'
    );
    expect(decoder.getDurationWithFilePath).not.toHaveBeenCalled();
  });

  it('exposes duration probing through BaseAudioContext', async () => {
    const decoder = createDecoder(3.25);
    installDecoder(decoder);

    const BaseAudioContext = require('../src/core/BaseAudioContext')
      .default as typeof import('../src/core/BaseAudioContext').default;

    const context = new BaseAudioContext({
      destination: createNativeAudioNode(),
      sampleRate: 44100,
      currentTime: 0,
      state: 'running',
    } as unknown as IBaseAudioContext);

    await expect(context.getAudioDuration('/tmp/audio.wav')).resolves.toBe(
      3.25
    );
    expect(decoder.getDurationWithFilePath).toHaveBeenCalledWith(
      '/tmp/audio.wav'
    );
  });
});
