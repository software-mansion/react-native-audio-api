import { IAudioNode } from '../interfaces';
import { ChannelCountMode, ChannelInterpretation } from './types';
import BaseAudioContext from './BaseAudioContext';

export default class AudioNode {
  readonly context: BaseAudioContext;
  readonly numberOfInputs: number;
  readonly numberOfOutputs: number;
  readonly channelCount: number;
  readonly channelCountMode: ChannelCountMode;
  readonly channelInterpretation: ChannelInterpretation;
  protected readonly node: IAudioNode;

  constructor(context: BaseAudioContext, node: IAudioNode) {
    this.context = context;
    this.node = node;
    this.numberOfInputs = this.node.numberOfInputs;
    this.numberOfOutputs = this.node.numberOfOutputs;
    this.channelCount = this.node.channelCount;
    this.channelCountMode = this.node.channelCountMode;
    this.channelInterpretation = this.node.channelInterpretation;
  }

  public connect(node: AudioNode): void {
    this.node.connect(node.node);
  }

  public disconnect(node: AudioNode): void {
    this.node.disconnect(node.node);
  }
}
