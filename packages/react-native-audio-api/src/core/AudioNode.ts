import { IAudioNode } from '../interfaces';
import AudioParam from './AudioParam';
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

  public connect(destination: AudioNode | AudioParam): AudioNode | AudioParam {
    if (this.context !== destination.context) {
      throw new InvalidAccessError(
        'Source and destination are from different BaseAudioContexts'
      );
    }

    if (destination instanceof AudioParam) {
      this.node.connect(destination.audioParam);
    } else {
      this.node.connect(destination.node);
    }

    return destination;
  }

  public disconnect(destination?: AudioNode | AudioParam): void {
    if (destination instanceof AudioParam) {
      this.node.disconnect(destination.audioParam);
    } else {
      this.node.disconnect(destination?.node);
    }
  }
}
