import { IAudioNode } from '../jsi-interfaces';
import AudioParam from './AudioParam';
import { ChannelCountMode, ChannelInterpretation } from '../types';
import type BaseAudioContext from './BaseAudioContext';
import { IndexSizeError } from '../errors';

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

  public connect(
    destination: AudioNode,
    output?: number,
    input?: number
  ): AudioNode;

  public connect(destination: AudioParam, output?: number): void;
  public connect(
    destination: AudioNode | AudioParam,
    output: number = 0,
    input: number = 0
  ): AudioNode | void {
    if (this.context !== destination.context) {
      throw new IndexSizeError(
        'Source and destination are from different BaseAudioContexts'
      );
    }

    if (output < 0 || output >= this.numberOfOutputs) {
      throw new IndexSizeError(
        `The output index provided (${output}) is outside the range [0, ${this.numberOfOutputs})`
      );
    }

    if (destination instanceof AudioParam) {
      this.node.connect(destination.audioParam, output);
    } else {
      if (input < 0 || input >= destination.numberOfInputs) {
        throw new IndexSizeError(
          `The input index provided (${input}) is outside the range [0, ${destination.numberOfInputs})`
        );
      }

      this.node.connect(destination.node, output, input);
      return destination;
    }
  }

  public disconnect(): void;
  public disconnect(output: number): void;
  public disconnect(
    destination: AudioNode,
    output?: number,
    input?: number
  ): void;

  public disconnect(destination: AudioParam, output?: number): void;
  public disconnect(
    destinationOrOutput?: AudioNode | AudioParam | number,
    output?: number,
    input?: number
  ): void {
    if (destinationOrOutput === undefined) {
      this.node.disconnect();
      return;
    }

    if (typeof destinationOrOutput === 'number') {
      this.validateOutput(destinationOrOutput);
      this.node.disconnect(destinationOrOutput);
      return;
    }

    if (output !== undefined) {
      this.validateOutput(output);
    }

    if (destinationOrOutput instanceof AudioParam) {
      this.node.disconnect(destinationOrOutput.audioParam, output);
      return;
    }

    if (
      input !== undefined &&
      (input < 0 || input >= destinationOrOutput.numberOfInputs)
    ) {
      throw new IndexSizeError(
        `The input index provided (${input}) is outside the range [0, ${destinationOrOutput.numberOfInputs})`
      );
    }

    this.node.disconnect(destinationOrOutput.node, output, input);
  }

  private validateOutput(output: number): void {
    if (output < 0 || output >= this.numberOfOutputs) {
      throw new IndexSizeError(
        `The output index provided (${output}) is outside the range [0, ${this.numberOfOutputs})`
      );
    }
  }
}
