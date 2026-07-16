import { IAudioNode } from '../jsi-interfaces';
import AudioParam from './AudioParam';
import { ChannelCountMode, ChannelInterpretation } from '../types';
import type BaseAudioContext from './BaseAudioContext';
import { IndexSizeError, InvalidAccessError } from '../errors';
import { validateChannelCount } from '../utils/validation/audioNodeOptions';

export default class AudioNode {
  readonly context: BaseAudioContext;
  readonly numberOfInputs: number;
  readonly numberOfOutputs: number;
  protected readonly node: IAudioNode;

  constructor(context: BaseAudioContext, node: IAudioNode) {
    this.context = context;
    this.node = node;
    this.numberOfInputs = this.node.numberOfInputs;
    this.numberOfOutputs = this.node.numberOfOutputs;
  }

  public get channelCount(): number {
    return this.node.channelCount;
  }

  public set channelCount(value: number) {
    validateChannelCount(value);
    this.node.channelCount = value;
  }

  public get channelCountMode(): ChannelCountMode {
    return this.node.channelCountMode;
  }

  public set channelCountMode(value: ChannelCountMode) {
    this.node.channelCountMode = value;
  }

  public get channelInterpretation(): ChannelInterpretation {
    return this.node.channelInterpretation;
  }

  public set channelInterpretation(value: ChannelInterpretation) {
    this.node.channelInterpretation = value;
  }

  public connect(destination: AudioNode): AudioNode;
  public connect(destination: AudioParam): void;
  public connect(destination: AudioNode | AudioParam): AudioNode | void {
    if (this.context !== destination.context) {
      throw new InvalidAccessError(
        'Source and destination are from different BaseAudioContexts'
      );
    }

    if (this.numberOfOutputs === 0) {
      throw new IndexSizeError('Faild to connect: AudioNode has no output');
    }

    if (destination instanceof AudioParam) {
      this.node.connect(destination.audioParam);
    } else {
      if (destination.numberOfInputs === 0) {
        throw new IndexSizeError(
          'Failed to connect: destination AudioNode has no input'
        );
      }

      this.node.connect(destination.node);
      return destination;
    }
  }

  public disconnect(destination?: AudioNode | AudioParam): void {
    if (destination !== undefined && this.context !== destination.context) {
      throw new InvalidAccessError(
        'Source and destination are from different BaseAudioContexts'
      );
    }

    if (destination instanceof AudioParam) {
      this.node.disconnect(destination.audioParam);
    } else {
      this.node.disconnect(destination?.node);
    }
  }
}
