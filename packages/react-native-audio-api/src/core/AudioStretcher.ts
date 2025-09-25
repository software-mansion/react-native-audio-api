import { IAudioStretcher } from '../interfaces';
import AudioBuffer from './AudioBuffer';

export default class AudioStretcher {
  protected readonly stretcher: IAudioStretcher;

  constructor(sampleRate: number) {
    this.stretcher = global.createAudioStretcher(sampleRate);
  }

  public async changePlaybackSpeed(
    input: AudioBuffer,
    playbackSpeed: number
  ): Promise<AudioBuffer> {
    const buffer = await this.stretcher.changePlaybackSpeed(
      input.buffer,
      playbackSpeed
    );

    if (!buffer) {
      throw new Error('Failed to change playback speed');
    }
    return new AudioBuffer(buffer);
  }
}
