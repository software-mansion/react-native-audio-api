import { IAudioBuffer, IOfflineAudioContext } from '../interfaces';
import BaseAudioContext from './BaseAudioContext';
import { OfflineAudioContextOptions } from '../types';
import { NotSupportedError } from '../errors';

export default class OfflineAudioContext extends BaseAudioContext {
  constructor(options: OfflineAudioContextOptions) {
    if (options.sampleRate < 8000 || options.sampleRate > 96000) {
      throw new NotSupportedError(
        `The provided sampleRate is not supported: ${options.sampleRate}`
      );
    }

    super(
      global.createOfflineAudioContext(
        options.numberOfChannels,
        options.length,
        options.sampleRate
      )
    );
  }

  async suspend(): Promise<undefined> {
    await (this.context as IOfflineAudioContext).suspend();
  }

  async resume(): Promise<undefined> {
    await (this.context as IOfflineAudioContext).resume();
  }

  async startRendering(): Promise<IAudioBuffer> {
    return await (this.context as IOfflineAudioContext).startRendering();
  }
}
