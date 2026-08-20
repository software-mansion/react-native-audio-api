import AudioParam from './AudioParam';
import type BaseAudioContext from './BaseAudioContext';
import { EventTypeWithValue } from '../events/types';
import { IAudioBufferBaseSourceNode } from '../jsi-interfaces';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import { AudioNodeOptions } from '../types';
import { AudioEventSubscription } from '../events';

export default class AudioBufferBaseSourceNode extends AudioScheduledSourceNode {
  readonly playbackRate: AudioParam;
  readonly detune: AudioParam;
  private onPositionChangedCallback?: (event: EventTypeWithValue) => void;
  private onPositionChangedSubscription: AudioEventSubscription | null = null;

  constructor(
    context: BaseAudioContext,
    node: IAudioBufferBaseSourceNode,
    options?: AudioNodeOptions
  ) {
    super(context, node, options);

    this.detune = new AudioParam(node.detune, context, this);
    this.playbackRate = new AudioParam(node.playbackRate, context, this);
  }

  public get onPositionChanged():
    | ((event: EventTypeWithValue) => void)
    | undefined {
    return this.onPositionChangedCallback;
  }

  public set onPositionChanged(
    callback: ((event: EventTypeWithValue) => void) | null
  ) {
    // See the note in AudioScheduledSourceNode.onEnded: the native registry holds a
    // strong reference, so the outgoing handler must be released here.
    this.onPositionChangedSubscription?.remove();
    this.onPositionChangedSubscription = null;

    if (!callback) {
      (this.node as IAudioBufferBaseSourceNode).onPositionChanged = '0';
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
