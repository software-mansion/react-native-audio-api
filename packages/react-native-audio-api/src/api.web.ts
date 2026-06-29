export { default as AudioBuffer } from './web-core/AudioBuffer.web';
export { default as AudioBufferSourceNode } from './web-core/AudioBufferSourceNode.web';
export { default as AudioContext } from './web-core/AudioContext.web';
export { default as OfflineAudioContext } from './web-core/OfflineAudioContext.web';
export { default as AudioDestinationNode } from './web-core/AudioDestinationNode.web';
export { default as AudioNode } from './web-core/AudioNode.web';
export { default as AnalyserNode } from './web-core/AnalyserNode.web';
export { default as AudioParam } from './web-core/AudioParam.web';
export { default as AudioScheduledSourceNode } from './web-core/AudioScheduledSourceNode.web';
export { default as BaseAudioContext } from './web-core/BaseAudioContext.web';
export { default as BiquadFilterNode } from './web-core/BiquadFilterNode.web';
export { default as DelayNode } from './web-core/DelayNode.web';
export { default as GainNode } from './web-core/GainNode.web';
export { default as MediaElementAudioSourceNode } from './web-core/MediaElementAudioSourceNode.web';
export type { MediaElementAudioSourceOptions } from './web-core/MediaElementAudioSourceNode.web';
export { default as OscillatorNode } from './web-core/OscillatorNode.web';
export { default as StereoPannerNode } from './web-core/StereoPannerNode.web';
export { default as ConstantSourceNode } from './web-core/ConstantSourceNode.web';
export { default as ConvolverNode } from './web-core/ConvolverNode.web';
export { default as PeriodicWave } from './web-core/PeriodicWave.web';
export { default as WaveShaperNode } from './web-core/WaveShaperNode.web';
export { default as IIRFilterNode } from './web-core/IIRFilterNode.web';
export {
  default as AudioDecoder,
  decodeAudioData,
  decodePCMInBase64,
} from './web-core/AudioDecoder.web';

export * from './web-core/custom';

// React components (Audio tag)
export * from './Audio';
export { default as Audio } from './Audio';
export { default as AudioControls } from './Audio/controls/AudioControls';

export function concatAudioFiles(
  _inputPaths: string[],
  _outputPath: string
): Promise<string> {
  return Promise.reject(new Error('concatAudioFiles is not supported on web.'));
}

export type {
  OscillatorType,
  ChannelCountMode,
  ChannelInterpretation,
  ContextState,
} from './types';

export type {
  IOSCategory,
  IOSMode,
  IOSOption,
  SessionOptions,
  PermissionStatus,
} from './system/types';

export * from './system/notification/types';
export type { default as AudioEventSubscription } from './events/AudioEventSubscription';

export {
  PlaybackNotificationManager,
  RecordingNotificationManager,
  AudioManager,
} from './web-system';

export {
  IndexSizeError,
  InvalidAccessError,
  InvalidStateError,
  RangeError,
  NotSupportedError,
} from './errors';
