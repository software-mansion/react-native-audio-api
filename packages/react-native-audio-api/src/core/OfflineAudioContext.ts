import { IOfflineAudioContext } from '../interfaces';
import BaseAudioContext from './BaseAudioContext';
import { OfflineAudioContextOptions } from '../types';
import { NotSupportedError } from '../errors';
import AudioBuffer from './AudioBuffer';

export default class OfflineAudioContext extends BaseAudioContext {
  constructor(options: OfflineAudioContextOptions);
  constructor(numberOfChannels: number, length: number, sampleRate: number);
  constructor(
    arg0: OfflineAudioContextOptions | number,
    arg1?: number,
    arg2?: number
  ) {
    if (typeof arg0 === 'object') {
      const { numberOfChannels, length, sampleRate } = arg0;
      super(
        global.createOfflineAudioContext(numberOfChannels, length, sampleRate)
      );
    } else if (
      typeof arg0 === 'number' &&
      typeof arg1 === 'number' &&
      typeof arg2 === 'number'
    ) {
      super(global.createOfflineAudioContext(arg0, arg1, arg2));
    } else {
      throw new NotSupportedError('Invalid constructor arguments');
    }
  }

  async resume(): Promise<undefined> {
    await (this.context as IOfflineAudioContext).resume();
  }

  async suspend(suspendTime: number): Promise<undefined> {
    await (this.context as IOfflineAudioContext).suspend(suspendTime);
  }

  async startRendering(): Promise<AudioBuffer> {
    const nativeAudioBuffer = await (
      this.context as IOfflineAudioContext
    ).startRendering();
    return new AudioBuffer(nativeAudioBuffer);
  }
}
