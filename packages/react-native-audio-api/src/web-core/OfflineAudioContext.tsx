import { OfflineAudioContextOptions } from '../types';
import { NotSupportedError, InvalidStateError } from '../errors';
import BaseAudioContext from './BaseAudioContext';
import AudioBuffer from './AudioBuffer';

export default class OfflineAudioContext extends BaseAudioContext {
  private isSuspended: boolean;
  private isRendering: boolean;
  private duration: number;

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
        new window.OfflineAudioContext(numberOfChannels, length, sampleRate)
      );

      this.duration = length / sampleRate;
    } else if (
      typeof arg0 === 'number' &&
      typeof arg1 === 'number' &&
      typeof arg2 === 'number'
    ) {
      super(new window.OfflineAudioContext(arg0, arg1, arg2));
      this.duration = arg1 / arg2;
    } else {
      throw new NotSupportedError('Invalid constructor arguments');
    }

    this.isSuspended = false;
    this.isRendering = false;
  }

  async resume(): Promise<undefined> {
    if (!this.isRendering) {
      throw new InvalidStateError(
        'Cannot resume an OfflineAudioContext while rendering'
      );
    }

    if (!this.isSuspended) {
      throw new InvalidStateError(
        'Cannot resume an OfflineAudioContext that is not suspended'
      );
    }

    this.isSuspended = false;

    await (this.context as globalThis.OfflineAudioContext).resume();
  }

  async suspend(suspendTime: number): Promise<undefined> {
    if (suspendTime < 0) {
      throw new InvalidStateError('suspendTime must be a non-negative number');
    }

    if (suspendTime < this.context.currentTime) {
      throw new InvalidStateError(
        `suspendTime must be greater than the current time: ${suspendTime}`
      );
    }

    if (suspendTime > this.duration) {
      throw new InvalidStateError(
        `suspendTime must be less than the duration of the context: ${suspendTime}`
      );
    }

    this.isSuspended = true;

    await (this.context as globalThis.OfflineAudioContext).suspend(suspendTime);
  }

  async startRendering(): Promise<AudioBuffer> {
    if (this.isRendering) {
      throw new InvalidStateError('OfflineAudioContext is already rendering');
    }

    this.isRendering = true;

    const audioBuffer = await (
      this.context as globalThis.OfflineAudioContext
    ).startRendering();

    return new AudioBuffer(audioBuffer);
  }
}
