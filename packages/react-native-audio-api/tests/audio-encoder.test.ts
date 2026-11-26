import AudioBuffer from '../src/core/AudioBuffer';
import { encodeAudioData } from '../src/core/AudioEncoder';
import type { IAudioEncoder } from '../src/jsi-interfaces';
import { FileDirectory, FileFormat } from '../src/types';
import FilePreset from '../src/utils/filePresets';

const encodedFile = {
  path: 'file:///cache/AudioAPI/encoded.m4a',
  size: 0.1,
  duration: 1,
};

const encoder = {
  encode: jest.fn().mockResolvedValue(encodedFile),
} as unknown as jest.Mocked<IAudioEncoder>;

beforeAll(() => {
  globalThis.createAudioEncoder = jest.fn(() => encoder);
  globalThis.createAudioBuffer = jest.fn(
    (numberOfChannels: number, length: number, sampleRate: number) => ({
      length,
      duration: length / sampleRate,
      sampleRate,
      numberOfChannels,
      getChannelData: jest.fn(() => new Float32Array(length).buffer),
      copyFromChannel: jest.fn(),
      copyToChannel: jest.fn(),
    })
  ) as unknown as typeof globalThis.createAudioBuffer;
});

beforeEach(() => encoder.encode.mockClear());

const createBuffer = () =>
  new AudioBuffer({ numberOfChannels: 2, length: 4, sampleRate: 44100 });

describe('encodeAudioData', () => {
  it('leaves the file layout to native when none is given', async () => {
    const buffer = createBuffer();

    await expect(encodeAudioData(buffer)).resolves.toEqual(encodedFile);

    expect(encoder.encode).toHaveBeenCalledWith([buffer.buffer], {
      format: FileFormat.M4A,
      channelCount: 0,
      preset: { ...FilePreset.High, sampleRate: 0 },
      directory: FileDirectory.Cache,
      subDirectory: 'AudioAPI',
      fileName: '',
    });
  });

  it('keeps the defaults for options given as undefined', async () => {
    const buffer = createBuffer();

    await encodeAudioData(buffer, {
      format: undefined,
      channelCount: undefined,
      preset: undefined,
      directory: undefined,
    });

    expect(encoder.encode).toHaveBeenCalledWith([buffer.buffer], {
      format: FileFormat.M4A,
      channelCount: 0,
      preset: { ...FilePreset.High, sampleRate: 0 },
      directory: FileDirectory.Cache,
      subDirectory: 'AudioAPI',
      fileName: '',
    });
  });

  it('takes the file rate from a given preset instead of the input', async () => {
    await encodeAudioData([createBuffer(), createBuffer()], {
      format: FileFormat.OpusOgg,
      preset: FilePreset.Low,
      channelCount: 1,
    });

    const [buffers, options] = encoder.encode.mock.calls[0];
    expect(buffers).toHaveLength(2);
    expect(options).toMatchObject({
      format: FileFormat.OpusOgg,
      channelCount: 1,
      preset: FilePreset.Low,
    });
  });

  it('hands out fresh channel views after the native snapshot', async () => {
    const buffer = createBuffer();
    const before = buffer.getChannelData(0);

    await encodeAudioData(buffer);

    expect(buffer.getChannelData(0)).not.toBe(before);
  });

  it('rejects an empty buffer list', async () => {
    await expect(encodeAudioData([])).rejects.toThrow(
      'encodeAudioData requires at least one AudioBuffer.'
    );
    expect(encoder.encode).not.toHaveBeenCalled();
  });
});
