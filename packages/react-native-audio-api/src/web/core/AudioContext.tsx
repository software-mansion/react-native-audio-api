import { AudioContextOptions } from '../../types';
import { NotSupportedError } from '../../errors';
import BaseAudioContext from './BaseAudioContext';

export default class AudioContext extends BaseAudioContext {
  constructor(options?: AudioContextOptions, _initSuspended: boolean = false) {
    if (
      options &&
      options.sampleRate &&
      (options.sampleRate < 8000 || options.sampleRate > 96000)
    ) {
      throw new NotSupportedError(
        `The provided sampleRate is not supported: ${options.sampleRate}`
      );
    }

    super(new window.AudioContext({ sampleRate: options?.sampleRate }));
  }

  async close(): Promise<undefined> {
    await (this.context as globalThis.AudioContext).close();
  }

  async resume(): Promise<undefined> {
    await (this.context as globalThis.AudioContext).resume();
  }

  async suspend(): Promise<undefined> {
    await (this.context as globalThis.AudioContext).suspend();
  }
}
