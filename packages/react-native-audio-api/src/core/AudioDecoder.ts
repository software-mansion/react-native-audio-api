import { IAudioDecoder } from '../interfaces';
import AudioBuffer from './AudioBuffer';

class AudioDecoder {
  private static instance: AudioDecoder | null = null;
  protected readonly decoder: IAudioDecoder;

  private constructor() {
    this.decoder = global.createAudioDecoder();
  }

  public static getInstance(): AudioDecoder {
    if (!AudioDecoder.instance) {
      AudioDecoder.instance = new AudioDecoder();
    }
    return AudioDecoder.instance;
  }

  public async decodeAudioDataInstance(
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
  input: string | ArrayBuffer,
  sampleRate?: number
): Promise<AudioBuffer> {
  return AudioDecoder.getInstance().decodeAudioDataInstance(input, sampleRate);
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
