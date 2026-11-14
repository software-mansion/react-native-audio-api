import { Image, Platform } from 'react-native';
import { NativeAudioAPIModule } from '../specs';

import { AudioApiError } from '../errors';
import { IAudioDecoder } from '../jsi-interfaces';
import { AudioDecodeStreamOptions, DecodeDataInput } from '../types';
import { isRemoteSource, resolveLocalFilePath } from '../utils/paths';
import { base64ToArrayBuffer } from '../utils';
import { assertSupportedDecodeStringSource } from '../utils/validation';
import AudioBuffer from './AudioBuffer';
import AudioDecodeStream from './AudioDecodeStream';

/** The encoded bytes of a source, or the path of the local file holding them. */
type EncodedSource = ArrayBuffer | string;

class AudioDecoder {
  private static instance: AudioDecoder | null = null;
  protected readonly decoder: IAudioDecoder;

  private constructor() {
    this.decoder = globalThis.createAudioDecoder();
  }

  private async decodeAudioDataImplementation(
    input: DecodeDataInput,
    sampleRate?: number,
    fetchOptions?: RequestInit
  ): Promise<AudioBuffer | null | undefined> {
    const rate = sampleRate ?? 0;
    const source = await this.loadEncodedSource(input, fetchOptions);

    if (source instanceof ArrayBuffer) {
      return this.decodeFromArrayBuffer(source, rate);
    }

    const buffer = await this.decoder.decodeWithFilePath(source, rate);
    return new AudioBuffer(buffer);
  }

  private async decodeFromArrayBuffer(
    arrayBuffer: ArrayBuffer,
    sampleRate: number
  ): Promise<AudioBuffer> {
    const buffer = await this.decoder.decodeWithMemoryBlock(
      // @ts-ignore internal function
      new Uint8Array(arrayBuffer),
      sampleRate
    );
    return new AudioBuffer(buffer);
  }

  private resolveToStringSource(input: number | string): string {
    if (typeof input === 'string') {
      return input;
    }

    const uri = Image.resolveAssetSource(input)?.uri;
    return uri ?? '';
  }

  /**
   * Resolves a decode input to what the native decoder opens: the encoded bytes
   * held in memory, or the path of a local file.
   */
  private async loadEncodedSource(
    input: DecodeDataInput,
    fetchOptions?: RequestInit
  ): Promise<EncodedSource> {
    if (input instanceof ArrayBuffer) {
      return input;
    }

    const stringSource = this.resolveToStringSource(input);
    assertSupportedDecodeStringSource(stringSource);

    if (isRemoteSource(stringSource)) {
      return fetch(stringSource, fetchOptions).then((res) => res.arrayBuffer());
    }

    const useAndroidBundledAssetReader =
      Platform.OS === 'android' &&
      !stringSource.startsWith('file://') &&
      !__DEV__;

    // special workaround, because android bundled assets are passed as f.e. asset_example_file
    // which has to be handled on native side and extension is not passed with it so decode using memory
    if (useAndroidBundledAssetReader) {
      const base64Payload =
        await NativeAudioAPIModule.readAndroidReleaseAssetBytesAsBase64(
          stringSource
        );
      return base64ToArrayBuffer(base64Payload);
    }

    return resolveLocalFilePath(stringSource);
  }

  public static getInstance(): AudioDecoder {
    if (!AudioDecoder.instance) {
      AudioDecoder.instance = new AudioDecoder();
    }

    return AudioDecoder.instance;
  }

  public async decodeAudioDataInstance(
    input: DecodeDataInput,
    sampleRate?: number,
    fetchOptions?: RequestInit
  ): Promise<AudioBuffer> {
    const audioBuffer = await this.decodeAudioDataImplementation(
      input,
      sampleRate,
      fetchOptions
    );

    if (!audioBuffer) {
      throw new AudioApiError('Failed to decode audio data.');
    }

    return audioBuffer;
  }

  public async openDecodeStreamInstance(
    input: DecodeDataInput,
    options: AudioDecodeStreamOptions = {}
  ): Promise<AudioDecodeStream> {
    const sampleRate = options.sampleRate ?? 0;
    const source = await this.loadEncodedSource(input, options.fetchOptions);

    const stream =
      source instanceof ArrayBuffer
        ? await this.decoder.openStreamWithMemoryBlock(
            // @ts-ignore internal function
            new Uint8Array(source),
            sampleRate
          )
        : await this.decoder.openStreamWithFilePath(source, sampleRate);
    return new AudioDecodeStream(stream);
  }

  public async decodePCMInBase64Instance(
    base64String: string,
    inputSampleRate: number,
    inputChannelCount: number,
    interleaved: boolean
  ): Promise<AudioBuffer> {
    const buffer = await this.decoder.decodeWithPCMInBase64(
      base64String,
      inputSampleRate,
      inputChannelCount,
      interleaved
    );
    return new AudioBuffer(buffer);
  }
}

export async function decodeAudioData(
  input: DecodeDataInput,
  sampleRate?: number,
  fetchOptions?: RequestInit
): Promise<AudioBuffer> {
  return AudioDecoder.getInstance().decodeAudioDataInstance(
    input,
    sampleRate,
    fetchOptions
  );
}

/**
 * Opens `input` to decode it a few frames at a time instead of as one
 * `AudioBuffer`. Takes the inputs `decodeAudioData` takes; close the stream
 * once done with it.
 */
export async function openAudioDecodeStream(
  input: DecodeDataInput,
  options?: AudioDecodeStreamOptions
): Promise<AudioDecodeStream> {
  return AudioDecoder.getInstance().openDecodeStreamInstance(input, options);
}

export async function decodePCMInBase64(
  base64String: string,
  inputSampleRate: number,
  inputChannelCount: number,
  isInterleaved: boolean = true
): Promise<AudioBuffer> {
  return AudioDecoder.getInstance().decodePCMInBase64Instance(
    base64String,
    inputSampleRate,
    inputChannelCount,
    isInterleaved
  );
}
