import { IAudioBufferBaseSourceNode } from '../interfaces';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import BaseAudioContext from './BaseAudioContext';
import AudioParam from './AudioParam';
import { EventTypeWithValue } from '../events/types';
import { AudioEventSubscription } from '../events';

export default class AudioBufferBaseSourceNode extends AudioScheduledSourceNode {
  readonly playbackRate: AudioParam;
  readonly detune: AudioParam;
  private positionChangedSubscription?: AudioEventSubscription;

  constructor(context: BaseAudioContext, node: IAudioBufferBaseSourceNode) {
    super(context, node);

    this.detune = new AudioParam(node.detune, context);
    this.playbackRate = new AudioParam(node.playbackRate, context);
  }

  // eslint-disable-next-line accessor-pairs
  public set onPositionChanged(
    callback: ((event: EventTypeWithValue) => void) | null
  ) {
    if (!callback) {
      this.positionChangedSubscription?.remove();
      this.positionChangedSubscription = undefined;
      (this.node as IAudioBufferBaseSourceNode).onPositionChanged = '0';
      return;
    }
    this.positionChangedSubscription =
      this.audioEventEmitter.addAudioEventListener('positionChanged', callback);

    (this.node as IAudioBufferBaseSourceNode).onPositionChanged =
      this.positionChangedSubscription.subscriptionId;
  }

  // eslint-disable-next-line accessor-pairs
  public set onPositionChangedInterval(value: number) {
    (this.node as IAudioBufferBaseSourceNode).onPositionChangedInterval = value;
  }
}
