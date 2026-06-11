import { AudioApiError } from '../errors';
import { DecodeDataInput } from '../types';
import { base64ToArrayBuffer } from '../utils';
import AudioBuffer from './AudioBuffer.web';
import OfflineAudioContext from './OfflineAudioContext.web';

const MAX_INT16_VALUE = 32768.0;

export default class AudioDecoder {
  private static instance: AudioDecoder | null = null;

  // keep it a singleton pattern
  // eslint-disable-next-line no-useless-constructor
  private constructor() {}

  public static getInstance(): AudioDecoder {
    if (!AudioDecoder.instance) {
      AudioDecoder.instance = new AudioDecoder();
    }

    return AudioDecoder.instance;
  }

  private async decodeAudioDataImplementation(
    input: DecodeDataInput,
    sampleRate?: number,
    fetchOptions?: RequestInit
  ): Promise<AudioBuffer | null | undefined> {
    const rate = sampleRate ?? 0;

    if (input instanceof ArrayBuffer) {
      return this.decodeFromArrayBuffer(input, rate);
    }

    if (typeof input !== 'string') {
      throw TypeError(
        'Input must be either an ArrayBuffer or a valid string URL path'
      );
    }

    return this.decodeFromRemoteUrl(input, rate, fetchOptions);
  }

  private async decodeFromArrayBuffer(
    arrayBuffer: ArrayBuffer,
    sampleRate: number
  ): Promise<AudioBuffer> {
    const targetRate = sampleRate > 0 ? sampleRate : 44100;
    const context = new OfflineAudioContext(1, 1, targetRate);
    return await context.decodeAudioData(arrayBuffer);
  }

  private async decodeFromRemoteUrl(
    url: string,
    sampleRate: number,
    fetchOptions?: RequestInit
  ): Promise<AudioBuffer> {
    const arrayBuffer = await fetch(url, fetchOptions).then((res) =>
      res.arrayBuffer()
    );
    return this.decodeFromArrayBuffer(arrayBuffer, sampleRate);
  }

  public async decodeAudioDataInstance(
    input: DecodeDataInput,
    sampleRate?: number,
    fetchOptions?: RequestInit
  ): Promise<AudioBuffer> {
    const audioBuffer = await this.decodeAudioDataImplementation(
      input,
      sampleRate,
      fetchOptions
    );

    if (!audioBuffer) {
      throw new AudioApiError('Failed to decode audio data.');
    }

    return audioBuffer;
  }

  public async decodePCMInBase64Instance(
    base64String: string,
    inputSampleRate: number,
    inputChannelCount: number,
    interleaved: boolean
  ): Promise<AudioBuffer> {
    try {
      const arrayBuffer = base64ToArrayBuffer(base64String);
      // map as 16 bits
      const int16samples = new Int16Array(arrayBuffer);
      const totalSampleCount = int16samples.length;
      const frameCount = totalSampleCount / inputChannelCount;

      // get requested buffer to write into
      const context = new OfflineAudioContext(
        inputChannelCount,
        frameCount,
        inputSampleRate
      );
      const buffer = context.createBuffer(
        inputChannelCount,
        frameCount,
        inputSampleRate
      );

      // deinterleave
      let outIndex: number;
      for (let channel = 0; channel < inputChannelCount; channel++) {
        const channelData = buffer.getChannelData(channel);
        for (let frameIndex = 0; frameIndex < frameCount; frameIndex++) {
          outIndex = interleaved
            ? channel + frameIndex * inputChannelCount // Ch1, Ch2, Ch1, Ch2, ...
            : frameIndex + channel * frameCount; // Ch1, Ch1, Ch1, ..., Ch2, Ch2, Ch2, ...

          // normalize
          channelData[frameIndex] = int16samples[outIndex] / MAX_INT16_VALUE;
        }
      }
      return Promise.resolve(buffer);
    } catch {
      throw new AudioApiError('Failed to decode PCM data.');
    }
  }
}

export async function decodeAudioData(
  input: DecodeDataInput,
  sampleRate?: number,
  fetchOptions?: RequestInit
): Promise<AudioBuffer> {
  return AudioDecoder.getInstance().decodeAudioDataInstance(
    input,
    sampleRate,
    fetchOptions
  );
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
