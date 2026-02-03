import { Image } from 'react-native';

import { AudioApiError } from '../errors';
import { IAudioDecoder } from '../interfaces';
import { DecodeDataInput } from '../types';
import {
  isBase64Source,
  isDataBlobString,
  isRemoteSource,
} from '../utils/paths';
import AudioBuffer from './AudioBuffer';

class AudioDecoder {
  private static instance: AudioDecoder | null = null;
  protected readonly decoder: IAudioDecoder;

  private constructor() {
    this.decoder = global.createAudioDecoder();
  }

  private async decodeAudioDataImplementation(
    input: DecodeDataInput,
    sampleRate?: number,
    fetchOptions?: RequestInit
  ): Promise<AudioBuffer | null | undefined> {
    if (input instanceof ArrayBuffer) {
      const buffer = await this.decoder.decodeWithMemoryBlock(
        new Uint8Array(input),
        sampleRate ?? 0
      );
      return new AudioBuffer(buffer);
    }

    const stringSource =
      typeof input === 'number' ? Image.resolveAssetSource(input).uri : input;

    // input is data:audio/...;base64,...
    if (isBase64Source(stringSource)) {
      throw new AudioApiError(
        'Base64 source decoding is not currently supported, to decode raw PCM base64 strings use decodePCMInBase64 method.'
      );
    }

    // input is blob:...
    if (isDataBlobString(stringSource)) {
      throw new AudioApiError(
        'Data Blob string decoding is not currently supported.'
      );
    }

    // input is http(s)://...
    if (isRemoteSource(stringSource)) {
      const arrayBuffer = await fetch(stringSource, fetchOptions).then((res) =>
        res.arrayBuffer()
      );

      const buffer = await this.decoder.decodeWithMemoryBlock(
        new Uint8Array(arrayBuffer),
        sampleRate ?? 0
      );

      return new AudioBuffer(buffer);
    }

    if (!(typeof stringSource === 'string')) {
      throw new TypeError('Input must be a module, uri or ArrayBuffer');
    }

    // Local file path
    const filePath = stringSource.startsWith('file://')
      ? stringSource.replace('file://', '')
      : stringSource;

    const buffer = await this.decoder.decodeWithFilePath(
      filePath,
      sampleRate ?? 0
    );

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
