import { NativeAudioAPIModule } from './specs';
import { AudioRecorderOptions } from './types';
import type {
  IAudioContext,
  IAudioRecorder,
  IOfflineAudioContext,
  IAudioEventEmitter,
} from './mobile/interfaces';

/* eslint-disable no-var */
declare global {
  var createAudioContext: (
    sampleRate: number,
    initSuspended: boolean
  ) => IAudioContext;
  var createOfflineAudioContext: (
    numberOfChannels: number,
    length: number,
    sampleRate: number
  ) => IOfflineAudioContext;

  var createAudioRecorder: (options: AudioRecorderOptions) => IAudioRecorder;

  var AudioEventEmitter: IAudioEventEmitter;
}
/* eslint-disable no-var */

if (
  global.createAudioContext == null ||
  global.createOfflineAudioContext == null ||
  global.createAudioRecorder == null ||
  global.AudioEventEmitter == null
) {
  if (!NativeAudioAPIModule) {
    throw new Error(
      `Failed to install react-native-audio-api: The native module could not be found.`
    );
  }

  NativeAudioAPIModule.install();
}

export { default as AudioBuffer } from './mobile/sources/AudioBuffer';
export { default as AudioBufferSourceNode } from './mobile/sources/AudioBufferSourceNode';
export { default as AudioBufferQueueSourceNode } from './mobile/sources/AudioBufferQueueSourceNode';
export { default as AudioContext } from './mobile/core/AudioContext';
export { default as OfflineAudioContext } from './mobile/core/OfflineAudioContext';
export { default as AudioDestinationNode } from './mobile/destinations/AudioDestinationNode';
export { default as AudioNode } from './mobile/core/AudioNode';
export { default as AnalyserNode } from './mobile/analysis/AnalyserNode';
export { default as AudioParam } from './mobile/core/AudioParam';
export { default as AudioScheduledSourceNode } from './mobile/sources/AudioScheduledSourceNode';
export { default as BaseAudioContext } from './mobile/core/BaseAudioContext';
export { default as BiquadFilterNode } from './mobile/effects/BiquadFilterNode';
export { default as CustomProcessorNode } from './mobile/effects/CustomProcessorNode';
export { default as GainNode } from './mobile/effects/GainNode';
export { default as OscillatorNode } from './mobile/sources/OscillatorNode';
export { default as StereoPannerNode } from './mobile/effects/StereoPannerNode';
export { default as AudioRecorder } from './mobile/inputs/AudioRecorder';
export { default as AudioManager } from './mobile/system';
export { default as useSystemVolume } from './mobile/hooks/useSystemVolume';

export {
  OscillatorType,
  BiquadFilterType,
  ChannelCountMode,
  ChannelInterpretation,
  ContextState,
  WindowType,
  PeriodicWaveConstraints,
} from './types';

export {
  IndexSizeError,
  InvalidAccessError,
  InvalidStateError,
  RangeError,
  NotSupportedError,
} from './errors';
