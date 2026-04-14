import { IAudioStretcher } from '../interfaces';
import AudioBuffer from './AudioBuffer';
import { AudioApiError } from '../errors';

class AudioStretcher {
  private static instance: AudioStretcher | null = null;
  protected readonly stretcher: IAudioStretcher;

  private constructor() {
    this.stretcher = global.createAudioStretcher();
  }

  public static getInstance(): AudioStretcher {
    if (!AudioStretcher.instance) {
      AudioStretcher.instance = new AudioStretcher();
    }
    return AudioStretcher.instance;
  }

  public async changePlaybackSpeedInstance(
    input: AudioBuffer,
    playbackSpeed: number
  ): Promise<AudioBuffer> {
    const buffer = await this.stretcher.changePlaybackSpeed(
      input.buffer,
      playbackSpeed
    );

    if (!buffer) {
      throw new AudioApiError('Failed to change playback speed');
    }
    return new AudioBuffer(buffer);
  }
}

export default async function changePlaybackSpeed(
  input: AudioBuffer,
  playbackSpeed: number
): Promise<AudioBuffer> {
  return AudioStretcher.getInstance().changePlaybackSpeedInstance(
    input,
    playbackSpeed
  );
}
