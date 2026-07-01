/* eslint-disable @typescript-eslint/no-var-requires */

import type { IAudioDecoder, IAudioFileUtils } from '../src/jsi-interfaces';

type AudioDecoderExports = typeof import('../src/core/AudioDecoder');
type AudioFileUtilsExports = typeof import('../src/core/AudioFileUtils');
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

const createDecoder = () =>
  ({
    decodeWithMemoryBlock: jest.fn(),
    decodeWithFilePath: jest.fn(),
    decodeWithPCMInBase64: jest.fn(),
  }) as unknown as jest.Mocked<IAudioDecoder>;

const createFileUtils = (duration: number = 12.5) =>
  ({
    concatAudioFiles: jest.fn(),
    probeDuration: jest.fn().mockResolvedValue(duration),
  }) as unknown as jest.Mocked<IAudioFileUtils>;

const installDecoder = (decoder: IAudioDecoder) => {
  globalThis.createAudioDecoder = jest.fn(() => decoder);
};

const installFileUtils = (fileUtils: IAudioFileUtils) => {
  globalThis.createAudioFileUtils = jest.fn(() => fileUtils);
};

const loadAudioFileUtils = (): AudioFileUtilsExports => {
  return require('../src/core/AudioFileUtils') as AudioFileUtilsExports;
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

  it('routes local file paths to native duration probing', async () => {
    const fileUtils = createFileUtils();
    installFileUtils(fileUtils);
    installDecoder(createDecoder());

    const { getAudioDuration } = loadAudioFileUtils();

    await expect(
      getAudioDuration('file:///tmp/audio%20file.wav')
    ).resolves.toBe(12.5);
    expect(fileUtils.probeDuration).toHaveBeenCalledWith(
      '/tmp/audio file.wav',
      0,
      undefined
    );
  });

  it('probes ArrayBuffer input through audio file utils', async () => {
    const fileUtils = createFileUtils(8.25);
    installFileUtils(fileUtils);
    installDecoder(createDecoder());

    const { getAudioDuration } = loadAudioFileUtils();
    const arrayBuffer = new ArrayBuffer(8);

    await expect(getAudioDuration(arrayBuffer)).resolves.toBe(8.25);
    expect(fileUtils.probeDuration).toHaveBeenCalledWith(
      arrayBuffer,
      undefined,
      undefined
    );
  });

  it('routes remote URLs to native duration probing', async () => {
    const fileUtils = createFileUtils(6.5);
    installFileUtils(fileUtils);
    installDecoder(createDecoder());

    const { getAudioDuration } = loadAudioFileUtils();

    await expect(
      getAudioDuration('https://example.com/audio.mp3', {
        headers: { Authorization: 'Bearer token' },
      })
    ).resolves.toBe(6.5);
    expect(fileUtils.probeDuration).toHaveBeenCalledWith(
      'https://example.com/audio.mp3',
      0,
      { Authorization: 'Bearer token' }
    );
  });

  it('rejects asset module ids without resolving bundled assets', async () => {
    installFileUtils(createFileUtils());
    installDecoder(createDecoder());

    const { getAudioDuration } = loadAudioFileUtils();

    await expect(getAudioDuration(1 as unknown as string)).rejects.toThrow(
      'Input must be a local file path, remote URL, or ArrayBuffer.'
    );
  });

  it('does not decode audio data when probing duration', async () => {
    const decoder = createDecoder();
    installDecoder(decoder);
    installFileUtils(createFileUtils());

    loadAudioFileUtils();
    loadAudioDecoder();

    expect(decoder.decodeWithFilePath).not.toHaveBeenCalled();
    expect(decoder.decodeWithMemoryBlock).not.toHaveBeenCalled();
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

  it('reads duration from ArrayBuffer input on web', async () => {
    installMockAudio({ duration: 3.5 });
    const { getAudioDuration } = loadWebAudioDecoder();

    await expect(getAudioDuration(new ArrayBuffer(8))).resolves.toBe(3.5);
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
