import { IAudioNode } from '../interfaces';
import { ChannelCountMode, ChannelInterpretation } from '../types';
import BaseAudioContext from './BaseAudioContext';
import { InvalidAccessError } from '../errors';

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

  public connect(destination: AudioNode): AudioNode {
    if (this.context !== destination.context) {
      throw new InvalidAccessError(
        'The AudioNodes are from different BaseAudioContexts'
      );
    }

    this.node.connect(destination.node);

    return destination;
  }

  public disconnect(destination?: AudioNode): void {
    this.node.disconnect(destination?.node);
  }
}
