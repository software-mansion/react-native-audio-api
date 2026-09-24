import type {
  IAudioContext,
  IAudioDecoder,
  IAudioEventEmitter,
  IAudioFileUtils,
  IAudioRecorder,
  IAudioBuffer,
  IOfflineAudioContext,
} from '../jsi-interfaces';
import type {
  AndroidOutputProfile,
  AudioRecorderOptions,
  FileInfo,
} from '../types';

/* eslint-disable no-var */
declare global {
  var createAudioContext: (
    sampleRate: number,
    androidOutputProfile?: AndroidOutputProfile
  ) => IAudioContext;
  var createOfflineAudioContext: (
    numberOfChannels: number,
    length: number,
    sampleRate: number
  ) => IOfflineAudioContext;

  var createAudioRecorder: (options: AudioRecorderOptions) => IAudioRecorder;

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
