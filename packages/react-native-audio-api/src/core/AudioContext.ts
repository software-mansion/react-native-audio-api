import { IAudioContext } from '../interfaces';
import BaseAudioContext from './BaseAudioContext';
import AudioManager from '../system';
import { AudioContextOptions } from '../types';
import { NotSupportedError } from '../errors';

export default class AudioContext extends BaseAudioContext {
  constructor(options?: AudioContextOptions, _initSuspended: boolean = false) {
    if (options && (options.sampleRate < 8000 || options.sampleRate > 96000)) {
      throw new NotSupportedError(
        `The provided sampleRate is not supported: ${options.sampleRate}`
      );
    }

    super(
      global.createAudioContext(
        options?.sampleRate || AudioManager.getDevicePreferredSampleRate(),
        _initSuspended
      )
    );
  }

  async close(): Promise<undefined> {
    await (this.context as IAudioContext).close();
  }

  async resume(): Promise<undefined> {
    await (this.context as IAudioContext).resume();
  }

  async suspend(): Promise<undefined> {
    await (this.context as IAudioContext).suspend();
  }
}
