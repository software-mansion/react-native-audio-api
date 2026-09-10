import { IAudioBufferQueueSourceNode } from '../jsi-interfaces';
import AudioBufferBaseSourceNode from './AudioBufferBaseSourceNode';
import AudioBuffer from './AudioBuffer';
import { InvalidStateError, RangeError } from '../errors';
import type BaseAudioContext from './BaseAudioContext';
import {
  AudioBufferQueueSourceOptions,
  AudioBufferQueueSourceState,
} from '../types';
import { OnBufferEndEventType } from '../events/types';
import { AudioEventSubscription } from '../events';

export default class AudioBufferQueueSourceNode extends AudioBufferBaseSourceNode {
  private onBufferEndedCallback?: (event: OnBufferEndEventType) => void;
  private onBufferEndedSubscription: AudioEventSubscription | null = null;
  private state: AudioBufferQueueSourceState = AudioBufferQueueSourceState.IDLE;

  constructor(
    context: BaseAudioContext,
    options?: AudioBufferQueueSourceOptions
  ) {
    const node = context.context.createBufferQueueSource(options || {});
    super(context, node, options);
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

  public override start(when: number = 0, offset: number = 0): void {
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

    if (this.state !== AudioBufferQueueSourceState.IDLE) {
      throw new InvalidStateError('Cannot call start more than once');
    }
    this.state = AudioBufferQueueSourceState.PLAYING;

    (this.node as IAudioBufferQueueSourceNode).start(when, offset);
    this.context.markRunningOnSourceStart();
  }

  public override stop(when: number = 0): void {
    if (when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }
    this.state = AudioBufferQueueSourceState.STOPPED;

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
    this.onBufferEndedSubscription?.remove();
    this.onBufferEndedSubscription = null;

    if (!callback) {
      (this.node as IAudioBufferQueueSourceNode).onBufferEnded = '0';
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
    if (this.state !== AudioBufferQueueSourceState.PLAYING) {
      throw new InvalidStateError(
        'Cannot call pause when the node is not playing'
      );
    }
    this.state = AudioBufferQueueSourceState.PAUSED;

    (this.node as IAudioBufferQueueSourceNode).pause();
  }

  public resume(when: number = 0): void {
    if (when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }

    if (this.state !== AudioBufferQueueSourceState.PAUSED) {
      throw new InvalidStateError(
        'Cannot call resume without calling pause first'
      );
    }
    this.state = AudioBufferQueueSourceState.PLAYING;

    (this.node as IAudioBufferQueueSourceNode).resume(when);
  }
}
