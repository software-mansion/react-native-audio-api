import { OfflineAudioContextOptions } from '../types';
import { NotSupportedError } from '../errors';
import { assertSupportedSampleRate } from '../utils/validation';
import BaseAudioContext from './BaseAudioContext.web';
import AudioBuffer from './AudioBuffer.web';

function createBrowserOfflineContext(
  arg0: OfflineAudioContextOptions | number,
  arg1?: number,
  arg2?: number
): globalThis.OfflineAudioContext {
  if (typeof arg0 === 'object') {
    assertSupportedSampleRate(arg0.sampleRate);
    return new window.OfflineAudioContext(arg0);
  }

  if (typeof arg1 === 'number' && typeof arg2 === 'number') {
    assertSupportedSampleRate(arg2);
    return new window.OfflineAudioContext(arg0, arg1, arg2);
  }

  throw new NotSupportedError('Invalid constructor arguments');
}

export default class OfflineAudioContext extends BaseAudioContext<globalThis.OfflineAudioContext> {
  constructor(options: OfflineAudioContextOptions);
  constructor(numberOfChannels: number, length: number, sampleRate: number);
  constructor(
    arg0: OfflineAudioContextOptions | number,
    arg1?: number,
    arg2?: number
  ) {
    super(createBrowserOfflineContext(arg0, arg1, arg2));
  }

  async decodeAudioDataSource(source: string): Promise<AudioBuffer> {
    return this.decodeAudioData(source);
  }

  async startRendering(): Promise<AudioBuffer> {
    return new AudioBuffer(await this.context.startRendering());
  }

  async resume(): Promise<void> {
    await this.context.resume();
  }

  async suspend(suspendTime: number): Promise<void> {
    await this.context.suspend(suspendTime);
  }
}
