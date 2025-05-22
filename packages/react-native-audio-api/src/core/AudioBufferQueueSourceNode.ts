import { IAudioBufferQueueSourceNode } from '../interfaces';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import BaseAudioContext from './BaseAudioContext';
import AudioBuffer from './AudioBuffer';
import AudioParam from './AudioParam';
import { InvalidStateError, RangeError } from '../errors';
import { OnPositionChangedEventType } from '../events/types';

export default class AudioBufferQueueSourceNode extends AudioScheduledSourceNode {
  readonly playbackRate: AudioParam;
  readonly detune: AudioParam;

  constructor(context: BaseAudioContext, node: IAudioBufferQueueSourceNode) {
    super(context, node);

    this.detune = new AudioParam(node.detune, context);
    this.playbackRate = new AudioParam(node.playbackRate, context);
  }

  public enqueueBuffer(
    buffer: AudioBuffer,
    bufferId: number = 0,
    isLastBuffer: boolean = false
  ): void {
    (this.node as IAudioBufferQueueSourceNode).enqueueBuffer(
      buffer.buffer,
      bufferId,
      isLastBuffer
    );
  }

  public start(when: number = 0, offset: number = 0): void {
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

    if (this.hasBeenStarted) {
      throw new InvalidStateError('Cannot call start more than once');
    }

    this.hasBeenStarted = true;
    (this.node as IAudioBufferQueueSourceNode).start(when, offset);
  }

  // eslint-disable-next-line accessor-pairs
  public set onPositionChanged(
    callback: (event: OnPositionChangedEventType) => void
  ) {
    const subscription = this.audioEventEmitter.addAudioEventListener(
      'positionChanged',
      callback
    );

    (this.node as IAudioBufferQueueSourceNode).onPositionChanged =
      subscription.subscriptionId;
  }

  // eslint-disable-next-line accessor-pairs
  public set onPositionChangedInterval(value: number) {
    (this.node as IAudioBufferQueueSourceNode).onPositionChangedInterval =
      value;
  }
}
