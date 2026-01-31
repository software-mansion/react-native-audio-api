import { IAudioBufferQueueSourceNode } from '../interfaces';
import AudioBufferBaseSourceNode from './AudioBufferBaseSourceNode';
import AudioBuffer from './AudioBuffer';
import { RangeError } from '../errors';
import BaseAudioContext from './BaseAudioContext';
import { TBaseAudioBufferSourceOptions } from '../types';
import { AudioEventSubscription } from '../events';
import { OnBufferEndEventType } from '../events/types';

export default class AudioBufferQueueSourceNode extends AudioBufferBaseSourceNode {
  private onBufferEndedSubscription?: AudioEventSubscription;
  private onBufferEndedCallback?: (event: OnBufferEndEventType) => void;

  constructor(
    context: BaseAudioContext,
    options?: TBaseAudioBufferSourceOptions
  ) {
    const node = context.context.createBufferQueueSource(options || {});
    super(context, node);
  }

  public enqueueBuffer(buffer: AudioBuffer): string {
    return (this.node as IAudioBufferQueueSourceNode).enqueueBuffer(
      buffer.buffer
    );
  }

  public dequeueBuffer(bufferId: string): void {
    const id = parseInt(bufferId, 10);
    if (isNaN(id) || id < 0) {
      throw new RangeError(
        `bufferId must be a non-negative integer: ${bufferId}`
      );
    }
    (this.node as IAudioBufferQueueSourceNode).dequeueBuffer(id);
  }

  public clearBuffers(): void {
    (this.node as IAudioBufferQueueSourceNode).clearBuffers();
  }

  public override start(when: number = 0, offset?: number): void {
    if (when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }

    if (offset && offset < 0) {
      throw new RangeError(
        `offset must be a finite non-negative number: ${offset}`
      );
    }

    (this.node as IAudioBufferQueueSourceNode).start(when, offset);
  }

  public override stop(when: number = 0): void {
    if (when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }

    (this.node as IAudioBufferQueueSourceNode).stop(when);
  }

  public get onBufferEnded():
    | ((event: OnBufferEndEventType) => void)
    | undefined {
    return this.onBufferEndedCallback;
  }

  public set onBufferEnded(
    callback: ((event: OnBufferEndEventType) => void) | null
  ) {
    if (!callback) {
      (this.node as IAudioBufferQueueSourceNode).onBufferEnded = '0';
      this.onBufferEndedSubscription?.remove();
      this.onBufferEndedSubscription = undefined;
      this.onBufferEndedCallback = undefined;

      return;
    }

    this.onBufferEndedCallback = callback;
    this.onBufferEndedSubscription =
      this.audioEventEmitter.addAudioEventListener('bufferEnded', callback);

    (this.node as IAudioBufferQueueSourceNode).onBufferEnded =
      this.onBufferEndedSubscription.subscriptionId;
  }

  public pause(): void {
    (this.node as IAudioBufferQueueSourceNode).pause();
  }
}
