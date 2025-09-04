import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';
import { AudioEventSubscription } from '../events';
import { EventTypeWithValue } from '../events/types';
import { IAudioBufferBaseSourceNode } from '../interfaces';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';

export default class AudioBufferBaseSourceNode extends AudioScheduledSourceNode {
  readonly playbackRate: AudioParam;
  readonly detune: AudioParam;
  private positionChangedSubscription?: AudioEventSubscription;
  private positionChangedCallback?: (event: EventTypeWithValue) => void;

  constructor(context: BaseAudioContext, node: IAudioBufferBaseSourceNode) {
    super(context, node);

    this.detune = new AudioParam(node.detune, context);
    this.playbackRate = new AudioParam(node.playbackRate, context);
  }

  public get onPositionChanged():
    | ((event: EventTypeWithValue) => void)
    | undefined {
    return this.positionChangedCallback;
  }

  public set onPositionChanged(
    callback: ((event: EventTypeWithValue) => void) | null
  ) {
    if (!callback) {
      (this.node as IAudioBufferBaseSourceNode).onPositionChanged = '0';
      this.positionChangedSubscription?.remove();
      this.positionChangedSubscription = undefined;
      this.positionChangedCallback = undefined;

      return;
    }

    this.positionChangedCallback = callback;
    this.positionChangedSubscription =
      this.audioEventEmitter.addAudioEventListener('positionChanged', callback);

    (this.node as IAudioBufferBaseSourceNode).onPositionChanged =
      this.positionChangedSubscription.subscriptionId;
  }

  public get onPositionChangedInterval(): number {
    return (this.node as IAudioBufferBaseSourceNode).onPositionChangedInterval;
  }

  public set onPositionChangedInterval(value: number) {
    (this.node as IAudioBufferBaseSourceNode).onPositionChangedInterval = value;
  }
}
