import { IAudioDecoder } from '../interfaces';
import AudioBuffer from './AudioBuffer';

export default class AudioDecoder {
  protected readonly decoder: IAudioDecoder;

  constructor(sampleRate: number) {
    this.decoder = global.createAudioDecoder(sampleRate);
  }

  public decodeAudioData(path: string): Promise<AudioBuffer>;
  public decodeAudioData(buffer: ArrayBuffer): Promise<AudioBuffer>;

  public async decodeAudioData(
    input: string | ArrayBuffer
  ): Promise<AudioBuffer> {
    let buffer;
    if (typeof input === 'string') {
      if (/\.(mp3|wav|flac|opus|ogg|m4a|aac|mp4)$/i.test(input)) {
        // Remove the file:// prefix if it exists
        if (input.startsWith('file://')) {
          input = input.replace('file://', '');
        }
        buffer = await this.decoder.decodeWithFilePath(input);
      } else {
        buffer = await this.decoder.decodeWithPCMInBase64(input);
      }
    } else if (input instanceof ArrayBuffer) {
      buffer = await this.decoder.decodeWithMemoryBlock(new Uint8Array(input));
    }

    if (!buffer) {
      throw new Error('Unsupported input type or failed to decode audio');
    }
    return new AudioBuffer(buffer);
  }
}
