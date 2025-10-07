import { IAudioContext } from '../interfaces';
import BaseAudioContext from './BaseAudioContext';
import AudioManager from '../system';
import { AudioContextOptions } from '../types';
import { NotSupportedError } from '../errors';
import { isWorkletsAvailable, workletsModule } from '../utils';

export default class AudioContext extends BaseAudioContext {
  // We need to keep here a reference to this runtime to better manage its lifecycle
  // eslint-disable-next-line @typescript-eslint/no-unused-vars, @typescript-eslint/no-explicit-any
  private _audioRuntime: any = null;

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
    if (isWorkletsAvailable) {
      audioRuntime = workletsModule.createWorkletRuntime('AudioWorkletRuntime');
    }

    super(
      global.createAudioContext(
        options?.sampleRate || AudioManager.getDevicePreferredSampleRate(),
        options?.initSuspended || false,
        audioRuntime
      )
    );
    this._audioRuntime = audioRuntime;
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
