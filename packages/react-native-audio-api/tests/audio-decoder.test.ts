/* eslint-disable @typescript-eslint/no-var-requires */

import type { IAudioDecoder } from '../src/jsi-interfaces';

type AudioDecoderExports = typeof import('../src/core/AudioDecoder');
type WebAudioDecoderExports = typeof import('../src/web-core/AudioDecoder.web');

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

const loadWebAudioDecoder = (): WebAudioDecoderExports => {
  return require('../src/web-core/AudioDecoder.web') as WebAudioDecoderExports;
};

const installMockAudio = ({
  duration,
  errorMessage,
  resetDurationOnCleanup = false,
}: {
  duration: number;
  errorMessage?: string;
  resetDurationOnCleanup?: boolean;
}) => {
  const listeners = new Map<string, () => void>();
  const audio = {
    duration,
    error: errorMessage ? { message: errorMessage } : null,
    preload: '',
    src: '',
    addEventListener: jest.fn((eventName: string, listener: () => void) => {
      listeners.set(eventName, listener);
    }),
    removeEventListener: jest.fn((eventName: string) => {
      listeners.delete(eventName);
    }),
    load: jest.fn(() => {
      if (audio.src === '') {
        if (resetDurationOnCleanup) {
          audio.duration = Number.NaN;
        }
        return;
      }

      const eventName = errorMessage ? 'error' : 'loadedmetadata';
      listeners.get(eventName)?.();
    }),
  } as unknown as HTMLAudioElement;
  const AudioMock = jest.fn(() => audio);

  (globalThis as unknown as { Audio: typeof Audio }).Audio =
    AudioMock as unknown as typeof Audio;

  return { audio, AudioMock };
};

describe('getAudioDuration', () => {
  const originalAudio = globalThis.Audio;

  beforeEach(() => {
    jest.resetModules();
  });

  afterEach(() => {
    if (originalAudio) {
      (globalThis as unknown as { Audio: typeof Audio }).Audio = originalAudio;
    } else {
      delete (globalThis as unknown as { Audio?: typeof Audio }).Audio;
    }
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

  it('reads duration from web audio metadata', async () => {
    const { audio, AudioMock } = installMockAudio({
      duration: 4.75,
      resetDurationOnCleanup: true,
    });
    const { getAudioDuration } = loadWebAudioDecoder();

    await expect(getAudioDuration('/audio/file.mp3')).resolves.toBe(4.75);

    expect(AudioMock).toHaveBeenCalledTimes(1);
    expect(audio.preload).toBe('metadata');
    expect(audio.addEventListener).toHaveBeenCalledWith(
      'loadedmetadata',
      expect.any(Function)
    );
    expect(audio.src).toBe('');
  });

  it('resolves zero-duration web audio metadata', async () => {
    installMockAudio({ duration: 0 });
    const { getAudioDuration } = loadWebAudioDecoder();

    await expect(getAudioDuration('/audio/empty.wav')).resolves.toBe(0);
  });

  it('rejects web audio sources when metadata has no finite duration', async () => {
    installMockAudio({ duration: Infinity });
    const { getAudioDuration } = loadWebAudioDecoder();

    await expect(getAudioDuration('/audio/live-stream.mp3')).rejects.toThrow(
      'Audio duration metadata is unavailable'
    );
  });

  it('rejects web audio sources when metadata loading fails', async () => {
    installMockAudio({ duration: 0, errorMessage: 'unsupported source' });
    const { getAudioDuration } = loadWebAudioDecoder();

    await expect(getAudioDuration('/audio/file.mp3')).rejects.toThrow(
      'Failed to load audio metadata: unsupported source'
    );
  });
});
