import { IAudioContext } from '../interfaces';
import BaseAudioContext from './BaseAudioContext';
import AudioManager from '../system';
import { AudioContextOptions } from '../types';
import { NotSupportedError } from '../errors';
import { isWorkletsVersionSupported, workletsModule } from '../utils';

export default class AudioContext extends BaseAudioContext {
  constructor(options?: AudioContextOptions) {
    if (
      options &&
      options.sampleRate &&
      (options.sampleRate < 8000 || options.sampleRate > 96000)
    ) {
      throw new NotSupportedError(
        `The provided sampleRate is not supported: ${options.sampleRate}`
      );
    }
    let audioRuntime = null;

    if (isWorkletsVersionSupported) {
      audioRuntime = workletsModule.createWorkletRuntime('AudioWorkletRuntime');
    }

    super(
      global.createAudioContext(
        options?.sampleRate || AudioManager.getDevicePreferredSampleRate(),
        options?.initSuspended || false,
        audioRuntime
      )
    );
  }

  async close(): Promise<void> {
    return (this.context as IAudioContext).close();
  }

  async resume(): Promise<boolean> {
    return (this.context as IAudioContext).resume();
  }

  async suspend(): Promise<boolean> {
    return (this.context as IAudioContext).suspend();
  }
}
