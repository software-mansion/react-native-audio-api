import { AudioApiError } from '../errors';
import { IAudioEncoder } from '../jsi-interfaces';
import { AudioEncoderOptions, EncodedFileInfo } from '../types';
import FilePreset, { withDefaultEncoderOptions } from '../utils/filePresets';
import AudioBuffer from './AudioBuffer';

class AudioEncoder {
  private static instance: AudioEncoder | null = null;
  protected readonly encoder: IAudioEncoder;

  private constructor() {
    this.encoder = globalThis.createAudioEncoder();
  }

  public static getInstance(): AudioEncoder {
    if (!AudioEncoder.instance) {
      AudioEncoder.instance = new AudioEncoder();
    }

    return AudioEncoder.instance;
  }

  public async encodeAudioDataInstance(
    input: AudioBuffer | AudioBuffer[],
    options: AudioEncoderOptions = {}
  ): Promise<EncodedFileInfo> {
    const buffers = Array.isArray(input) ? input : [input];

    if (buffers.length === 0) {
      throw new AudioApiError(
        'encodeAudioData requires at least one AudioBuffer.'
      );
    }

    const promise = this.encoder.encode(
      buffers.map((buffer) => buffer.buffer),
      withDefaultEncoderOptions({
        ...options,
        channelCount: options.channelCount ?? 0,
        preset: options.preset ?? { ...FilePreset.High, sampleRate: 0 },
      })
    );

    // Native snapshotted the samples, cutting off the views handed out so far.
    buffers.forEach((buffer) => buffer.invalidateChannelDataCache());

    return promise;
  }
}

/**
 * Encodes one or more AudioBuffers, back to back, into a single audio file.
 * Buffers may differ in sample rate and channel count; each is resampled and
 * remixed to the file's.
 *
 * @returns The written file.
 */
export async function encodeAudioData(
  input: AudioBuffer | AudioBuffer[],
  options?: AudioEncoderOptions
): Promise<EncodedFileInfo> {
  return AudioEncoder.getInstance().encodeAudioDataInstance(input, options);
}
