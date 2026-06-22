import { AudioApiError } from '../errors';
import { IAudioFileUtils } from '../jsi-interfaces';
import { prefetchFileSegments } from '../utils/metadataPrefetching';

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
    data: ArrayBuffer,
    sampleRate?: number
  ): Promise<number | null> {
    return this.fileUtils.probeDuration(data, sampleRate);
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
  url: string,
  startBytes: number,
  endBytes: number,
  sampleRate?: number,
  headers?: { [key: string]: string }
): Promise<number | null> {
  const prefetchedData = await prefetchFileSegments({
    url,
    startBytes,
    endBytes,
    headers,
  });
  return AudioFileUtils.getInstance().probeDurationInstance(
    prefetchedData,
    sampleRate
  );
}
