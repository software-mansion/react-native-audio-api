import { AudioApiError } from '../errors';
import { IAudioFileUtils } from '../interfaces';

class AudioFileUtils {
  private static instance: AudioFileUtils | null = null;
  protected readonly fileUtils: IAudioFileUtils;

  private constructor() {
    this.fileUtils = global.createAudioFileUtils();
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
