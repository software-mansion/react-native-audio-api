import type {
  IAudioConsumer,
  IAudioContext,
  IAudioDecoder,
  IAudioEventEmitter,
  IAudioFileUtils,
  IAudioRecorder,
  IAudioRecorderCallbackOptions,
  IAudioBuffer,
  IOfflineAudioContext,
} from '../jsi-interfaces';
import type {
  AndroidOutputProfile,
  AudioContextLatencyCategory,
  AudioRecorderOptions,
  FileInfo,
} from '../types';

/* eslint-disable no-var */
declare global {
  var createAudioContext: (
    sampleRate: number,
    latencyHint?: AudioContextLatencyCategory,
    androidOutputProfile?: AndroidOutputProfile
  ) => IAudioContext;
  var createOfflineAudioContext: (
    numberOfChannels: number,
    length: number,
    sampleRate: number
  ) => IOfflineAudioContext;

  var createAudioRecorder: (options: AudioRecorderOptions) => IAudioRecorder;

  /** The consumer behind `AudioRecorder.onAudioReady`. */
  var createAudioReadyConsumer: (
    options: IAudioRecorderCallbackOptions
  ) => IAudioConsumer;

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
