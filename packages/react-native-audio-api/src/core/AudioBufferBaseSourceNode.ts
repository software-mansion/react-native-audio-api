import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';
import { AudioEventSubscription } from '../events';
import { EventTypeWithValue } from '../events/types';
import { IAudioBufferBaseSourceNode } from '../interfaces';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';

export default class AudioBufferBaseSourceNode extends AudioScheduledSourceNode {
  readonly playbackRate: AudioParam;
  readonly detune: AudioParam;
  private onPositionChangedSubscription?: AudioEventSubscription;
  private onPositionChangedCallback?: (event: EventTypeWithValue) => void;

  constructor(context: BaseAudioContext, node: IAudioBufferBaseSourceNode) {
    super(context, node);

    this.detune = new AudioParam(node.detune, context);
    this.playbackRate = new AudioParam(node.playbackRate, context);
  }

  public get onPositionChanged():
    | ((event: EventTypeWithValue) => void)
    | undefined {
    return this.onPositionChangedCallback;
  }

  public set onPositionChanged(
    callback: ((event: EventTypeWithValue) => void) | null
  ) {
    if (!callback) {
      (this.node as IAudioBufferBaseSourceNode).onPositionChanged = '0';
      this.onPositionChangedSubscription?.remove();
      this.onPositionChangedSubscription = undefined;
      this.onPositionChangedCallback = undefined;

      return;
    }

    this.onPositionChangedCallback = callback;
    this.onPositionChangedSubscription =
      this.audioEventEmitter.addAudioEventListener('positionChanged', callback);

    (this.node as IAudioBufferBaseSourceNode).onPositionChanged =
      this.onPositionChangedSubscription.subscriptionId;
  }

  public get onPositionChangedInterval(): number {
    return (this.node as IAudioBufferBaseSourceNode).onPositionChangedInterval;
  }

  public set onPositionChangedInterval(value: number) {
    (this.node as IAudioBufferBaseSourceNode).onPositionChangedInterval = value;
  }

  public getLatency(): number {
    return (
      (this.node as IAudioBufferBaseSourceNode).getOutputLatency() +
      (this.node as IAudioBufferBaseSourceNode).getInputLatency() *
        (this.node as IAudioBufferBaseSourceNode).playbackRate.value
    );
  }
}
