import { IAudioContext } from '../interfaces';
import BaseAudioContext from './BaseAudioContext';
import { AudioContextOptions } from '../types';
import { NotSupportedError } from '../errors';

export default class AudioContext extends BaseAudioContext {
  constructor(options?: AudioContextOptions) {
    if (options && (options.sampleRate < 8000 || options.sampleRate > 96000)) {
      throw new NotSupportedError(
        `The provided sampleRate is not supported: ${options.sampleRate}`
      );
    }

    super(global.createAudioContext(options?.sampleRate));
  }

  async close(): Promise<undefined> {
    await (this.context as IAudioContext).close();
  }
}
