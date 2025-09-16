import { IAudioDecoder } from '../interfaces';
import AudioBuffer from './AudioBuffer';

export default class AudioDecoder {
  protected readonly decoder: IAudioDecoder;

  constructor(sampleRate: number) {
    this.decoder = global.createAudioDecoder(sampleRate);
  }

  public decode(path: string): Promise<AudioBuffer>;
  public decode(buffer: ArrayBuffer): Promise<AudioBuffer>;

  public decode(input: string | ArrayBuffer): Promise<AudioBuffer> {
    if (typeof input === 'string') {
      if (input.match(/\.(mp3|wav|flac|opus|ogg|m4a|aac|mp4)$/i)) {
        return this.decoder.decodeWithFilePath(input);
      }
      return this.decoder.decodeWithPCMInBase64(input);
    }

    if (input instanceof ArrayBuffer) {
      return this.decoder.decodeWithMemoryBlock(input);
    }

    return Promise.reject(new Error('Unsupported input type for decode()'));
  }
}
