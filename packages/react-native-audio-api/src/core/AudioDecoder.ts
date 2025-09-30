import { IAudioDecoder } from '../interfaces';
import AudioBuffer from './AudioBuffer';

export default class AudioDecoder {
  protected readonly decoder: IAudioDecoder;

  constructor() {
    this.decoder = global.createAudioDecoder();
  }

  public decodeAudioData(
    path: string,
    sampleRate?: number
  ): Promise<AudioBuffer>;

  public decodeAudioData(
    buffer: ArrayBuffer,
    sampleRate?: number
  ): Promise<AudioBuffer>;

  public async decodeAudioData(
    input: string | ArrayBuffer,
    sampleRate?: number
  ): Promise<AudioBuffer> {
    let buffer;
    if (typeof input === 'string') {
      // Remove the file:// prefix if it exists
      if (input.startsWith('file://')) {
        input = input.replace('file://', '');
      }
      buffer = await this.decoder.decodeWithFilePath(input, sampleRate ?? 0);
    } else if (input instanceof ArrayBuffer) {
      buffer = await this.decoder.decodeWithMemoryBlock(
        new Uint8Array(input),
        sampleRate ?? 0
      );
    }

    if (!buffer) {
      throw new Error('Unsupported input type or failed to decode audio');
    }
    return new AudioBuffer(buffer);
  }

  public async decodePCMInBase64(
    base64String: string,
    inputSampleRate: number,
    inputChannelCount: number,
    interleaved: boolean = true
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
