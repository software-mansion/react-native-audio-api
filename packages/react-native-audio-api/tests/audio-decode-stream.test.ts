/* eslint-disable @typescript-eslint/no-var-requires */

import type {
  IAudioBuffer,
  IAudioDecodeStream,
  IAudioDecoder,
} from '../src/jsi-interfaces';

type AudioDecoderExports = typeof import('../src/core/AudioDecoder');
type WebAudioDecodeStreamExports =
  typeof import('../src/web-core/AudioDecodeStream.web');
type WebAudioBufferExports = typeof import('../src/web-core/AudioBuffer.web');

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

const createNativeBuffer = (length: number): IAudioBuffer =>
  ({
    length,
    duration: length / 44100,
    sampleRate: 44100,
    numberOfChannels: 2,
    getChannelData: jest.fn(),
    copyFromChannel: jest.fn(),
    copyToChannel: jest.fn(),
  }) as unknown as IAudioBuffer;

const createNativeStream = () =>
  ({
    sampleRate: 44100,
    numberOfChannels: 2,
    duration: 12.5,
    read: jest.fn(),
    seek: jest.fn().mockResolvedValue(undefined),
    close: jest.fn(),
  }) as unknown as jest.Mocked<IAudioDecodeStream>;

const installDecoder = (stream: IAudioDecodeStream) => {
  const decoder = {
    decodeWithMemoryBlock: jest.fn(),
    decodeWithFilePath: jest.fn(),
    decodeWithPCMInBase64: jest.fn(),
    openStreamWithMemoryBlock: jest.fn().mockResolvedValue(stream),
    openStreamWithFilePath: jest.fn().mockResolvedValue(stream),
  } as unknown as jest.Mocked<IAudioDecoder>;
  globalThis.createAudioDecoder = jest.fn(() => decoder);
  return decoder;
};

const loadAudioDecoder = (): AudioDecoderExports => {
  return require('../src/core/AudioDecoder') as AudioDecoderExports;
};

describe('openAudioDecodeStream', () => {
  beforeEach(() => {
    jest.resetModules();
  });

  it('opens a local file at the requested sample rate', async () => {
    const decoder = installDecoder(createNativeStream());
    const { openAudioDecodeStream } = loadAudioDecoder();

    const stream = await openAudioDecodeStream('file:///tmp/audio%20file.wav', {
      sampleRate: 22050,
    });

    expect(decoder.openStreamWithFilePath).toHaveBeenCalledWith(
      '/tmp/audio file.wav',
      22050
    );
    expect(stream.sampleRate).toBe(44100);
    expect(stream.numberOfChannels).toBe(2);
    expect(stream.duration).toBe(12.5);
  });

  it('opens encoded bytes held in memory', async () => {
    const decoder = installDecoder(createNativeStream());
    const { openAudioDecodeStream } = loadAudioDecoder();
    const arrayBuffer = new ArrayBuffer(8);

    await openAudioDecodeStream(arrayBuffer);

    expect(decoder.openStreamWithMemoryBlock).toHaveBeenCalledTimes(1);
    const [bytes, sampleRate] = decoder.openStreamWithMemoryBlock.mock
      .calls[0] as unknown as [Uint8Array, number];
    expect(bytes.buffer).toBe(arrayBuffer);
    expect(sampleRate).toBe(0);
  });

  it('downloads a remote source before opening it', async () => {
    const decoder = installDecoder(createNativeStream());
    const { openAudioDecodeStream } = loadAudioDecoder();
    const arrayBuffer = new ArrayBuffer(8);
    const fetchOptions = { headers: { Authorization: 'token' } };
    const fetchMock = jest
      .fn()
      .mockResolvedValue({ arrayBuffer: () => Promise.resolve(arrayBuffer) });
    globalThis.fetch = fetchMock as unknown as typeof fetch;

    await openAudioDecodeStream('https://example.com/audio.mp3', {
      fetchOptions,
    });

    expect(fetchMock).toHaveBeenCalledWith(
      'https://example.com/audio.mp3',
      fetchOptions
    );
    expect(decoder.openStreamWithMemoryBlock).toHaveBeenCalledTimes(1);
    expect(decoder.openStreamWithFilePath).not.toHaveBeenCalled();
  });

  it('rejects when the source cannot be opened', async () => {
    const decoder = installDecoder(createNativeStream());
    decoder.openStreamWithFilePath.mockRejectedValue(
      new Error('Cannot decode file: unsupported or invalid audio format')
    );
    const { openAudioDecodeStream } = loadAudioDecoder();

    await expect(openAudioDecodeStream('/tmp/broken.wav')).rejects.toThrow(
      'Cannot decode file'
    );
  });
});

describe('AudioDecodeStream', () => {
  beforeEach(() => {
    jest.resetModules();
  });

  const openStream = async (nativeStream: IAudioDecodeStream) => {
    installDecoder(nativeStream);
    return loadAudioDecoder().openAudioDecodeStream('/tmp/audio.wav');
  };

  it('wraps each chunk in an AudioBuffer', async () => {
    const nativeStream = createNativeStream();
    nativeStream.read.mockResolvedValue(createNativeBuffer(4096));
    const stream = await openStream(nativeStream);

    const chunk = await stream.read(4096);

    expect(nativeStream.read).toHaveBeenCalledWith(4096);
    expect(chunk?.length).toBe(4096);
    expect(chunk?.numberOfChannels).toBe(2);
  });

  it('resolves null once the source has ended', async () => {
    const nativeStream = createNativeStream();
    nativeStream.read.mockResolvedValue(null);
    const stream = await openStream(nativeStream);

    await expect(stream.read(4096)).resolves.toBeNull();
  });

  it.each([0, -1, 1.5, Number.NaN, Number.POSITIVE_INFINITY])(
    'rejects a read of %p frames without reaching the decoder',
    async (frameCount) => {
      const nativeStream = createNativeStream();
      const stream = await openStream(nativeStream);

      await expect(stream.read(frameCount)).rejects.toThrow(
        'frameCount must be a positive integer'
      );
      expect(nativeStream.read).not.toHaveBeenCalled();
    }
  );

  it('seeks to a position in seconds', async () => {
    const nativeStream = createNativeStream();
    const stream = await openStream(nativeStream);

    await stream.seek(1.5);

    expect(nativeStream.seek).toHaveBeenCalledWith(1.5);
  });

  it.each([-1, Number.NaN, Number.POSITIVE_INFINITY])(
    'rejects a seek to %p without reaching the decoder',
    async (seconds) => {
      const nativeStream = createNativeStream();
      const stream = await openStream(nativeStream);

      await expect(stream.seek(seconds)).rejects.toThrow(
        'seconds must be a non-negative finite number'
      );
      expect(nativeStream.seek).not.toHaveBeenCalled();
    }
  );

  it('closes the native stream', async () => {
    const nativeStream = createNativeStream();
    const stream = await openStream(nativeStream);

    stream.close();

    expect(nativeStream.close).toHaveBeenCalledTimes(1);
  });
});

describe('AudioDecodeStream on web', () => {
  const originalAudioBuffer = globalThis.AudioBuffer;

  /** The part of the browser AudioBuffer the web stream relies on. */
  class BrowserAudioBufferStub {
    readonly length: number;
    readonly sampleRate: number;
    readonly numberOfChannels: number;
    readonly duration: number;
    private readonly channels: Float32Array[];

    constructor(options: {
      length: number;
      numberOfChannels: number;
      sampleRate: number;
    }) {
      this.length = options.length;
      this.sampleRate = options.sampleRate;
      this.numberOfChannels = options.numberOfChannels;
      this.duration = options.length / options.sampleRate;
      this.channels = Array.from(
        { length: options.numberOfChannels },
        () => new Float32Array(options.length)
      );
    }

    getChannelData(channel: number): Float32Array {
      return this.channels[channel];
    }

    copyToChannel(source: Float32Array, channel: number): void {
      this.channels[channel].set(source);
    }
  }

  const sampleRate = 8000;
  const length = 10;

  const createStream = () => {
    const { default: AudioBuffer } =
      require('../src/web-core/AudioBuffer.web') as WebAudioBufferExports;
    const { default: AudioDecodeStream } =
      require('../src/web-core/AudioDecodeStream.web') as WebAudioDecodeStreamExports;

    const decoded = new AudioBuffer({
      length,
      numberOfChannels: 1,
      sampleRate,
    });
    decoded.getChannelData(0).set(Array.from({ length }, (_, i) => i));
    return new AudioDecodeStream(decoded);
  };

  beforeEach(() => {
    jest.resetModules();
    (globalThis as unknown as { AudioBuffer: unknown }).AudioBuffer =
      BrowserAudioBufferStub;
  });

  afterEach(() => {
    (globalThis as unknown as { AudioBuffer: unknown }).AudioBuffer =
      originalAudioBuffer;
  });

  it('hands out the decoded audio in consecutive chunks', async () => {
    const stream = createStream();

    const first = await stream.read(4);
    const second = await stream.read(4);
    const last = await stream.read(4);

    expect(Array.from(first!.getChannelData(0))).toEqual([0, 1, 2, 3]);
    expect(Array.from(second!.getChannelData(0))).toEqual([4, 5, 6, 7]);
    expect(Array.from(last!.getChannelData(0))).toEqual([8, 9]);
    await expect(stream.read(4)).resolves.toBeNull();
  });

  it('reads from the position it was moved to', async () => {
    const stream = createStream();

    await stream.seek(6 / sampleRate);
    const chunk = await stream.read(2);

    expect(Array.from(chunk!.getChannelData(0))).toEqual([6, 7]);
  });

  it('rejects reads and seeks once closed', async () => {
    const stream = createStream();

    stream.close();

    await expect(stream.read(4)).rejects.toThrow('closed decode stream');
    await expect(stream.seek(0)).rejects.toThrow('closed decode stream');
  });
});
