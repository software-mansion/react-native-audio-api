import { IAudioBufferBaseSourceNode } from '../interfaces';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import BaseAudioContext from './BaseAudioContext';
import AudioParam from './AudioParam';
import { EventTypeWithValue } from '../events/types';

export default class AudioBufferBaseSourceNode extends AudioScheduledSourceNode {
  readonly playbackRate: AudioParam;
  readonly detune: AudioParam;

  constructor(context: BaseAudioContext, node: IAudioBufferBaseSourceNode) {
    super(context, node);

    this.detune = new AudioParam(node.detune, context);
    this.playbackRate = new AudioParam(node.playbackRate, context);
  }

  // eslint-disable-next-line accessor-pairs
  public set onPositionChanged(callback: (event: EventTypeWithValue) => void) {
    const subscription = this.audioEventEmitter.addAudioEventListener(
      'positionChanged',
      callback
    );

    (this.node as IAudioBufferBaseSourceNode).onPositionChanged =
      subscription.subscriptionId;
  }

  // eslint-disable-next-line accessor-pairs
  public set onPositionChangedInterval(value: number) {
    (this.node as IAudioBufferBaseSourceNode).onPositionChangedInterval = value;
  }
}
