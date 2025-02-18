import BaseAudioContext from './BaseAudioContext';
import { ChannelCountMode, ChannelInterpretation } from '../types';

export default class AudioNode {
  readonly context: BaseAudioContext;
  readonly numberOfInputs: number;
  readonly numberOfOutputs: number;
  readonly channelCount: number;
  readonly channelCountMode: ChannelCountMode;
  readonly channelInterpretation: ChannelInterpretation;

  protected readonly node: globalThis.AudioNode;

  constructor(context: BaseAudioContext, node: globalThis.AudioNode) {
    this.context = context;
    this.node = node;
    this.numberOfInputs = this.node.numberOfInputs;
    this.numberOfOutputs = this.node.numberOfOutputs;
    this.channelCount = this.node.channelCount;
    this.channelCountMode = this.node.channelCountMode;
    this.channelInterpretation = this.node.channelInterpretation;
  }

  public connect(destination: AudioNode): AudioNode {
    if (this.context !== destination.context) {
      throw new Error('The AudioNodes are from different BaseAudioContexts');
    }

    this.node.connect(destination.node);

    return destination;
  }

  public disconnect(destination?: AudioNode): void {
    if (destination === undefined) {
      this.node.disconnect();
      return;
    }

    this.node.disconnect(destination.node);
  }
}
