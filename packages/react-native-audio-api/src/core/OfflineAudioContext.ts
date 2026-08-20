import { InvalidStateError, NotSupportedError } from '../errors';
import { assertSupportedSampleRate } from '../utils/validation';
import { IOfflineAudioContext } from '../jsi-interfaces';
import { OfflineAudioContextOptions } from '../types';
import AudioBuffer from './AudioBuffer';
import BaseAudioContext from './BaseAudioContext';

export interface OfflineAudioCompletionEvent {
  type: 'complete';
  target: OfflineAudioContext;
  renderedBuffer: AudioBuffer;
}

export default class OfflineAudioContext extends BaseAudioContext {
  private isRendering: boolean;
  private duration: number;

  /**
   * Web Audio API `complete` event handler, dispatched when startRendering()
   * finishes. Kept alongside the promise because plenty of code (and the WPT
   * suite) never awaits the promise and relies on this event alone.
   */
  public oncomplete: ((event: OfflineAudioCompletionEvent) => void) | null;

  constructor(options: OfflineAudioContextOptions);
  constructor(numberOfChannels: number, length: number, sampleRate: number);
  constructor(
    arg0: OfflineAudioContextOptions | number,
    arg1?: number,
    arg2?: number
  ) {
    if (typeof arg0 === 'object') {
      const { numberOfChannels, length, sampleRate } = arg0;
      assertSupportedSampleRate(sampleRate);
      super(
        globalThis.createOfflineAudioContext(
          numberOfChannels,
          length,
          sampleRate
        )
      );

      this.duration = length / sampleRate;
    } else if (
      typeof arg0 === 'number' &&
      typeof arg1 === 'number' &&
      typeof arg2 === 'number'
    ) {
      assertSupportedSampleRate(arg2);
      super(globalThis.createOfflineAudioContext(arg0, arg1, arg2));
      this.duration = arg1 / arg2;
    } else {
      throw new NotSupportedError('Invalid constructor arguments');
    }

    this.isRendering = false;
    this.oncomplete = null;
  }

  async resume(): Promise<undefined> {
    if (!this.isRendering) {
      throw new InvalidStateError(
        'Cannot resume an OfflineAudioContext while rendering'
      );
    }

    if (!(this._state === 'suspended')) {
      throw new InvalidStateError(
        'Cannot resume an OfflineAudioContext that is not suspended'
      );
    }

    this.setControlState('running');
    await (this.context as IOfflineAudioContext).resume();
    this.publishState('running');
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

    if (this._state === 'closed') {
      throw new InvalidStateError('the rendering is already finished');
    }

    // The suspend promise resolves when rendering reaches the suspend point —
    // the acknowledgment the spec publishes the state change on.
    const result = await (this.context as IOfflineAudioContext).suspend(
      suspendTime
    );
    this.publishState('suspended');
    return result;
  }

  async startRendering(): Promise<AudioBuffer> {
    if (this.isRendering) {
      throw new InvalidStateError('OfflineAudioContext is already rendering');
    }

    this.isRendering = true;
    this.publishState('running');
    const audioBuffer = await (
      this.context as IOfflineAudioContext
    ).startRendering();
    this.publishState('closed');

    const renderedBuffer = new AudioBuffer(audioBuffer);
    // A task, not a microtask: `statechange` (queued by publishState above)
    // must fire before `complete`, per the spec's ordering.
    setTimeout(() => {
      this.oncomplete?.({
        type: 'complete',
        target: this,
        renderedBuffer,
      });
    }, 0);

    return renderedBuffer;
  }
}
