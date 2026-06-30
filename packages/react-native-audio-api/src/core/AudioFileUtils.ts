import { AudioApiError } from '../errors';
import { IAudioFileUtils } from '../jsi-interfaces';
import { AudioDurationInput } from '../types';
import { headersFromRequestInit } from '../utils';
import {
  isBase64Source,
  isDataBlobString,
  isRemoteSource,
  resolveLocalFilePath,
} from '../utils/paths';

class AudioFileUtils {
  private static instance: AudioFileUtils | null = null;
  protected readonly fileUtils: IAudioFileUtils;

  private constructor() {
    this.fileUtils = globalThis.createAudioFileUtils();
  }

  public static getInstance(): AudioFileUtils {
    if (!AudioFileUtils.instance) {
      AudioFileUtils.instance = new AudioFileUtils();
    }
    return AudioFileUtils.instance;
  }

  public async concatAudioFilesInstance(
    inputPaths: string[],
    outputPath: string
  ): Promise<string> {
    if (!Array.isArray(inputPaths) || inputPaths.length === 0) {
      throw new AudioApiError(
        'concatAudioFiles requires at least one input path.'
      );
    }

    if (inputPaths.some((inputPath) => typeof inputPath !== 'string')) {
      throw new TypeError('concatAudioFiles input paths must be strings.');
    }

    if (typeof outputPath !== 'string' || outputPath.length === 0) {
      throw new AudioApiError('concatAudioFiles requires an output path.');
    }

    return this.fileUtils.concatAudioFiles(inputPaths, outputPath);
  }

  public async probeDurationInstance(
    input: ArrayBuffer | string,
    sampleRate?: number,
    headers?: Record<string, string>
  ): Promise<number | null> {
    return this.fileUtils.probeDuration(input, sampleRate, headers);
  }

  public async getAudioDurationInstance(
    input: AudioDurationInput,
    fetchOptions?: RequestInit
  ): Promise<number> {
    let duration: number | null;

    if (input instanceof ArrayBuffer) {
      duration = await this.probeDurationInstance(input);
    } else if (typeof input !== 'string') {
      throw new TypeError(
        'Input must be a local file path, remote URL, or ArrayBuffer.'
      );
    } else {
      if (isBase64Source(input)) {
        throw new AudioApiError(
          'Base64 source decoding is not currently supported, to decode raw PCM base64 strings use decodePCMInBase64 method.'
        );
      }

      if (isDataBlobString(input)) {
        throw new AudioApiError(
          'Data Blob string decoding is not currently supported.'
        );
      }

      const headers = headersFromRequestInit(fetchOptions);
      const source = isRemoteSource(input)
        ? input
        : resolveLocalFilePath(input);
      duration = await this.probeDurationInstance(source, 0, headers);
    }

    if (duration === null) {
      throw new AudioApiError('Audio duration metadata is unavailable');
    }

    return duration;
  }
}

export async function concatAudioFiles(
  inputPaths: string[],
  outputPath: string
): Promise<string> {
  return AudioFileUtils.getInstance().concatAudioFilesInstance(
    inputPaths,
    outputPath
  );
}

export async function probeDuration(
  input: ArrayBuffer | string,
  sampleRate?: number,
  headers?: Record<string, string>
): Promise<number | null> {
  return AudioFileUtils.getInstance().probeDurationInstance(
    input,
    sampleRate,
    headers
  );
}

export async function getAudioDuration(
  input: AudioDurationInput,
  fetchOptions?: RequestInit
): Promise<number> {
  return AudioFileUtils.getInstance().getAudioDurationInstance(
    input,
    fetchOptions
  );
}
