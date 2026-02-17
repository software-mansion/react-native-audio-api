import type {
  IAudioContext,
  IAudioDecoder,
  IAudioEventEmitter,
  IAudioRecorder,
  IAudioBuffer,
  IAudioStretcher,
  IOfflineAudioContext,
} from '../interfaces';

/* eslint-disable no-var */
declare global {
  var createAudioContext: (
    sampleRate: number,
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    audioWorkletRuntime: any
  ) => IAudioContext;
  var createOfflineAudioContext: (
    numberOfChannels: number,
    length: number,
    sampleRate: number,
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    audioWorkletRuntime: any
  ) => IOfflineAudioContext;

  var createAudioRecorder: () => IAudioRecorder;

  var createAudioBuffer: (
    numberOfChannels: number,
    length: number,
    sampleRate: number
  ) => IAudioBuffer;

  var createAudioDecoder: () => IAudioDecoder;

  var createAudioStretcher: () => IAudioStretcher;

  var AudioEventEmitter: IAudioEventEmitter;
}
/* eslint-disable no-var */
