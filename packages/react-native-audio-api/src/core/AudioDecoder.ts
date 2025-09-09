import { IAudioDecoder } from '../interfaces';
import AudioBuffer from './AudioBuffer';

export default class AudioDecoder {
  protected readonly decoder: IAudioDecoder;

  constructor(sampleRate: number) {
    this.decoder = global.createAudioDecoder(sampleRate);
  }

  public decodeWithFilePath(path: string): Promise<AudioBuffer> {
    return this.decoder.decodeWithFilePath(path);
  }

  public decodeWithMemoryBlock(buffer: ArrayBuffer): Promise<AudioBuffer> {
    return this.decoder.decodeWithMemoryBlock(buffer);
  }

  public decodeWithPCMInBase64(
    data: string,
    playbackSpeed: number = 1
  ): Promise<AudioBuffer> {
    return this.decoder.decodeWithPCMInBase64(data, playbackSpeed);
  }
}
