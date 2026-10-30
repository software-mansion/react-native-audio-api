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
  private onpositionchangedCallback?: (event: EventTypeWithValue) => void;
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

  public get onpositionchanged():
    | ((event: EventTypeWithValue) => void)
    | undefined {
    return this.onpositionchangedCallback;
  }

  public set onpositionchanged(
    callback: ((event: EventTypeWithValue) => void) | null
  ) {
    this.onPositionChangedSubscription?.remove();
    this.onPositionChangedSubscription = null;

    if (!callback) {
      (this.node as IAudioBufferBaseSourceNode).onpositionchanged = '0';
      this.onpositionchangedCallback = undefined;
      return;
    }

    this.onpositionchangedCallback = callback;
    this.onPositionChangedSubscription =
      this.audioEventEmitter.addAudioEventListener('positionChanged', callback);

    (this.node as IAudioBufferBaseSourceNode).onpositionchanged =
      this.onPositionChangedSubscription.subscriptionId;
  }

  public get onpositionchangedInterval(): number {
    return (this.node as IAudioBufferBaseSourceNode).onpositionchangedInterval;
  }

  public set onpositionchangedInterval(value: number) {
    (this.node as IAudioBufferBaseSourceNode).onpositionchangedInterval = value;
  }

  public getLatency(): number {
    return (
      (this.node as IAudioBufferBaseSourceNode).getOutputLatency() +
      (this.node as IAudioBufferBaseSourceNode).getInputLatency() *
        (this.node as IAudioBufferBaseSourceNode).playbackRate.value
    );
  }
}
