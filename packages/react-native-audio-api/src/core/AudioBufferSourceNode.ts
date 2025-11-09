import { IAudioBufferSourceNode } from '../interfaces';
import AudioBufferBaseSourceNode from './AudioBufferBaseSourceNode';
import AudioBuffer from './AudioBuffer';
import { InvalidStateError, RangeError } from '../errors';
import { EventEmptyType } from '../events/types';
import { AudioEventSubscription } from '../events';

export default class AudioBufferSourceNode extends AudioBufferBaseSourceNode {
  private onLoopEndedSubscription?: AudioEventSubscription;
  private onLoopEndedCallback?: (event: EventEmptyType) => void;

  public get buffer(): AudioBuffer | null {
    const buffer = (this.node as IAudioBufferSourceNode).buffer;
    if (!buffer) {
      return null;
    }
    return new AudioBuffer(buffer);
  }

  public set buffer(buffer: AudioBuffer | null) {
    if (!buffer) {
      (this.node as IAudioBufferSourceNode).setBuffer(null);
      return;
    }

    (this.node as IAudioBufferSourceNode).setBuffer(buffer.buffer);
  }

  public get loopSkip(): boolean {
    return (this.node as IAudioBufferSourceNode).loopSkip;
  }

  public set loopSkip(value: boolean) {
    (this.node as IAudioBufferSourceNode).loopSkip = value;
  }

  public get loop(): boolean {
    return (this.node as IAudioBufferSourceNode).loop;
  }

  public set loop(value: boolean) {
    (this.node as IAudioBufferSourceNode).loop = value;
  }

  public get loopStart(): number {
    return (this.node as IAudioBufferSourceNode).loopStart;
  }

  public set loopStart(value: number) {
    (this.node as IAudioBufferSourceNode).loopStart = value;
  }

  public get loopEnd(): number {
    return (this.node as IAudioBufferSourceNode).loopEnd;
  }

  public set loopEnd(value: number) {
    (this.node as IAudioBufferSourceNode).loopEnd = value;
  }

  public start(when: number = 0, offset: number = 0, duration?: number): void {
    if (when < 0) {
      throw new RangeError(
        `when must be a finite non-negative number: ${when}`
      );
    }

    if (offset < 0) {
      throw new RangeError(
        `offset must be a finite non-negative number: ${offset}`
      );
    }

    if (duration && duration < 0) {
      throw new RangeError(
        `duration must be a finite non-negative number: ${duration}`
      );
    }

    if (this.hasBeenStarted) {
      throw new InvalidStateError('Cannot call start more than once');
    }

    this.hasBeenStarted = true;
    (this.node as IAudioBufferSourceNode).start(when, offset, duration);
  }

  public override get onEnded(): ((event: EventEmptyType) => void) | undefined {
    return super.onEnded as ((event: EventEmptyType) => void) | undefined;
  }

  public override set onEnded(
    callback: ((event: EventEmptyType) => void) | null
  ) {
    super.onEnded = callback;
  }

  public get onLoopEnded(): ((event: EventEmptyType) => void) | undefined {
    return this.onLoopEndedCallback;
  }

  public set onLoopEnded(callback: ((event: EventEmptyType) => void) | null) {
    if (!callback) {
      (this.node as IAudioBufferSourceNode).onLoopEnded = '0';
      this.onLoopEndedSubscription?.remove();
      this.onLoopEndedSubscription = undefined;
      this.onLoopEndedCallback = undefined;

      return;
    }

    this.onLoopEndedCallback = callback;
    this.onLoopEndedSubscription = this.audioEventEmitter.addAudioEventListener(
      'loopEnded',
      callback
    );

    (this.node as IAudioBufferSourceNode).onLoopEnded =
      this.onLoopEndedSubscription.subscriptionId;
  }
}
