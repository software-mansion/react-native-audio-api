import { IAudioBufferSourceNode } from '../jsi-interfaces';
import AudioBufferBaseSourceNode from './AudioBufferBaseSourceNode';
import AudioBuffer from './AudioBuffer';
import { InvalidStateError, RangeError } from '../errors';
import { EventEmptyType } from '../events/types';
import { AudioBufferSourceOptions } from '../types';
import type BaseAudioContext from './BaseAudioContext';
import { AudioEventSubscription } from '../events';

export default class AudioBufferSourceNode extends AudioBufferBaseSourceNode {
  private onLoopEndedCallback?: (event: EventEmptyType) => void;
  private onLoopEndedSubscription: AudioEventSubscription | null = null;

  private _buffer: AudioBuffer | null = null;
  private bufferHasBeenSet: boolean = false;

  constructor(context: BaseAudioContext, options?: AudioBufferSourceOptions) {
    // The native layer expects the AudioBuffer HostObject, not the TS wrapper,
    // so pass the buffer through the setter below instead of the options.
    const { buffer, ...nativeOptions } = options ?? {};
    const node = context.context.createBufferSource(nativeOptions);
    super(context, node, options);

    if (buffer != null) {
      this.buffer = buffer as AudioBuffer;
    }
  }

  public get buffer(): AudioBuffer | null {
    return this._buffer;
  }

  public set buffer(buffer: AudioBuffer | null) {
    if (buffer === null) {
      if (this.buffer !== null) {
        (this.node as IAudioBufferSourceNode).setBuffer(null);
        this._buffer = null;
      }

      return;
    }

    if (!(buffer instanceof AudioBuffer)) {
      throw new TypeError(
        'Failed to set buffer: the provided value is not of type AudioBuffer.'
      );
    }

    if (this.bufferHasBeenSet) {
      throw new InvalidStateError(
        'The buffer can only be set once and cannot be changed afterwards.'
      );
    }

    (this.node as IAudioBufferSourceNode).setBuffer(buffer.buffer);
    this._buffer = buffer;
    this.bufferHasBeenSet = true;
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
    this.context.markRunningOnSourceStart();
  }

  public get onLoopEnded(): ((event: EventEmptyType) => void) | undefined {
    return this.onLoopEndedCallback;
  }

  public set onLoopEnded(callback: ((event: EventEmptyType) => void) | null) {
    this.onLoopEndedSubscription?.remove();
    this.onLoopEndedSubscription = null;

    if (!callback) {
      (this.node as IAudioBufferSourceNode).onLoopEnded = '0';
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
