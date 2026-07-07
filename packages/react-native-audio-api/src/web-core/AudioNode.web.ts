import BaseAudioContext from './BaseAudioContext.web';
import { ChannelCountMode, ChannelInterpretation } from '../types';
import AudioParam from './AudioParam.web';

export default class AudioNode {
  readonly context: BaseAudioContext;
  readonly numberOfInputs: number;
  readonly numberOfOutputs: number;

  readonly node: globalThis.AudioNode;

  constructor(context: BaseAudioContext, node: globalThis.AudioNode) {
    this.context = context;
    this.node = node;
    this.numberOfInputs = this.node.numberOfInputs;
    this.numberOfOutputs = this.node.numberOfOutputs;
  }

  public get channelCount(): number {
    return this.node.channelCount;
  }

  public set channelCount(value: number) {
    this.node.channelCount = value;
  }

  public get channelCountMode(): ChannelCountMode {
    return this.node.channelCountMode as ChannelCountMode;
  }

  public set channelCountMode(value: ChannelCountMode) {
    this.node.channelCountMode = value;
  }

  public get channelInterpretation(): ChannelInterpretation {
    return this.node.channelInterpretation as ChannelInterpretation;
  }

  public set channelInterpretation(value: ChannelInterpretation) {
    this.node.channelInterpretation = value;
  }

  public connect(destination: AudioNode | AudioParam): AudioNode | AudioParam {
    if (this.context !== destination.context) {
      throw new Error(
        'Source and destination are from different BaseAudioContexts'
      );
    }

    if (destination instanceof AudioParam) {
      this.node.connect(destination.param);
    } else {
      this.node.connect(destination.node);
    }

    return destination;
  }

  public disconnect(destination?: AudioNode | AudioParam): void {
    if (destination === undefined) {
      this.node.disconnect();
      return;
    }

    if (destination instanceof AudioParam) {
      this.node.disconnect(destination.param);
      return;
    }
    this.node.disconnect(destination.node);
  }
}
