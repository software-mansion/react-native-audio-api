import { Image, Platform } from 'react-native';
import { NativeAudioAPIModule } from '../specs';

import { AudioApiError } from '../errors';
import { IAudioDecoder } from '../jsi-interfaces';
import { DecodeDataInput } from '../types';
import { isRemoteSource, resolveLocalFilePath } from '../utils/paths';
import { base64ToArrayBuffer } from '../utils';
import { assertSupportedDecodeStringSource } from '../utils/validation';
import AudioBuffer from './AudioBuffer';

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

    if (input instanceof ArrayBuffer) {
      return this.decodeFromArrayBuffer(input, rate);
    }

    const stringSource = this.resolveToStringSource(input);
    assertSupportedDecodeStringSource(stringSource);

    if (isRemoteSource(stringSource)) {
      return this.decodeFromRemoteUrl(stringSource, rate, fetchOptions);
    }

    return this.decodeFromLocalFile(stringSource, rate);
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
    return typeof input === 'number'
      ? Image.resolveAssetSource(input).uri
      : input;
  }

  private async decodeFromRemoteUrl(
    url: string,
    sampleRate: number,
    fetchOptions?: RequestInit
  ): Promise<AudioBuffer> {
    const arrayBuffer = await fetch(url, fetchOptions).then((res) =>
      res.arrayBuffer()
    );
    return this.decodeFromArrayBuffer(arrayBuffer, sampleRate);
  }

  private async decodeFromLocalFile(
    stringSource: string,
    sampleRate: number
  ): Promise<AudioBuffer> {
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
      const arrayBuffer = base64ToArrayBuffer(base64Payload);
      return this.decodeFromArrayBuffer(arrayBuffer, sampleRate);
    }

    const filePath = resolveLocalFilePath(stringSource);
    const buffer = await this.decoder.decodeWithFilePath(filePath, sampleRate);
    return new AudioBuffer(buffer);
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
