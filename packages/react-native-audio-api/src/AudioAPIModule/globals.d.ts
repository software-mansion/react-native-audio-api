import type {
  IAudioContext,
  IAudioDecoder,
  IAudioEventEmitter,
  IAudioFileUtils,
  IAudioRecorder,
  IAudioBuffer,
  IOfflineAudioContext,
} from '../jsi-interfaces';
import type { FileInfo } from '../types';

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

  var isRecordingOngoing: (() => boolean) | undefined;

  var consumeLastRecordingResult: (() => FileInfo | null) | undefined;

  var createAudioBuffer: (
    numberOfChannels: number,
    length: number,
    sampleRate: number
  ) => IAudioBuffer;

  var createAudioDecoder: () => IAudioDecoder;

  var createAudioFileUtils: () => IAudioFileUtils;

  var AudioEventEmitter: IAudioEventEmitter;
}
/* eslint-disable no-var */
