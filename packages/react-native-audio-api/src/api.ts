import { NativeAudioAPIModule } from './specs';
import { AudioRecorderOptions } from './types';
import type {
  IAudioContext,
  IAudioDecoder,
  IAudioRecorder,
  IAudioStretcher,
  IOfflineAudioContext,
  IAudioEventEmitter,
} from './interfaces';

/* eslint-disable no-var */
declare global {
  var createAudioContext: (
    sampleRate: number,
    initSuspended: boolean,
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

  var createAudioRecorder: (options: AudioRecorderOptions) => IAudioRecorder;

  var createAudioDecoder: () => IAudioDecoder;

  var createAudioStretcher: () => IAudioStretcher;

  var AudioEventEmitter: IAudioEventEmitter;
}
/* eslint-disable no-var */

if (
  global.createAudioContext == null ||
  global.createOfflineAudioContext == null ||
  global.createAudioRecorder == null ||
  global.createAudioDecoder == null ||
  global.createAudioStretcher == null ||
  global.AudioEventEmitter == null
) {
  if (!NativeAudioAPIModule) {
    throw new Error(
      `Failed to install react-native-audio-api: The native module could not be found.`
    );
  }

  NativeAudioAPIModule.install();
}

export { default as WorkletNode } from './core/WorkletNode';
export { default as WorkletSourceNode } from './core/WorkletSourceNode';
export { default as WorkletProcessingNode } from './core/WorkletProcessingNode';
export { default as RecorderAdapterNode } from './core/RecorderAdapterNode';
export { default as AudioBuffer } from './core/AudioBuffer';
export { default as AudioBufferSourceNode } from './core/AudioBufferSourceNode';
export { default as AudioBufferQueueSourceNode } from './core/AudioBufferQueueSourceNode';
export { default as AudioContext } from './core/AudioContext';
export { default as OfflineAudioContext } from './core/OfflineAudioContext';
export { default as AudioDestinationNode } from './core/AudioDestinationNode';
export { default as AudioNode } from './core/AudioNode';
export { default as AnalyserNode } from './core/AnalyserNode';
export { default as AudioParam } from './core/AudioParam';
export { default as AudioScheduledSourceNode } from './core/AudioScheduledSourceNode';
export { default as BaseAudioContext } from './core/BaseAudioContext';
export { default as BiquadFilterNode } from './core/BiquadFilterNode';
export { default as GainNode } from './core/GainNode';
export { default as OscillatorNode } from './core/OscillatorNode';
export { default as StereoPannerNode } from './core/StereoPannerNode';
export { default as AudioRecorder } from './core/AudioRecorder';
export { default as StreamerNode } from './core/StreamerNode';
export { default as ConstantSourceNode } from './core/ConstantSourceNode';
export { default as AudioManager } from './system';
export { default as ConvolverNode } from './core/ConvolverNode';
export { default as useSystemVolume } from './hooks/useSystemVolume';
export { decodeAudioData, decodePCMInBase64 } from './core/AudioDecoder';
export { default as changePlaybackSpeed } from './core/AudioStretcher';

export {
  OscillatorType,
  BiquadFilterType,
  ChannelCountMode,
  ChannelInterpretation,
  ContextState,
  WindowType,
  PeriodicWaveConstraints,
  AudioWorkletRuntime,
} from './types';

export {
  IOSCategory,
  IOSMode,
  IOSOption,
  SessionOptions,
  MediaState,
  LockScreenInfo,
  PermissionStatus,
} from './system/types';

export {
  IndexSizeError,
  InvalidAccessError,
  InvalidStateError,
  RangeError,
  NotSupportedError,
} from './errors';
