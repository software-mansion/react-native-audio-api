import NativeAudioAPIModule from './specs/NativeAudioAPIModule';
import type { IAudioContext } from './interfaces';
import type { IAudioManager } from './system';

/* eslint-disable no-var */
declare global {
  var createAudioContext: (sampleRate?: number) => IAudioContext;
  var AudioManager: IAudioManager;
}
/* eslint-disable no-var */

if (global.createAudioContext == null) {
  if (!NativeAudioAPIModule) {
    throw new Error(
      `Failed to install react-native-audio-api: The native module could not be found.`
    );
  }

  NativeAudioAPIModule.install();
}

export { default as AudioBuffer } from './core/AudioBuffer';
export { default as AudioBufferSourceNode } from './core/AudioBufferSourceNode';
export { default as AudioContext } from './core/AudioContext';
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
export { default as AudioManager } from './system';

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
