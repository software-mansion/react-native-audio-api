import { IAudioNode } from '../jsi-interfaces';
import AudioParam from './AudioParam';
import {
  AudioNodeOptions,
  ChannelCountMode,
  ChannelInterpretation,
} from '../types';
import type BaseAudioContext from './BaseAudioContext';
import { IndexSizeError, InvalidAccessError } from '../errors';
import {
  validateAudioNodeOptions,
  validateChannelCount,
} from '../utils/validation/audioNodeOptions';

export default class AudioNode {
  readonly context: BaseAudioContext;
  readonly numberOfInputs: number;
  readonly numberOfOutputs: number;
  protected readonly node: IAudioNode;

  constructor(
    context: BaseAudioContext,
    node: IAudioNode,
    options?: AudioNodeOptions
  ) {
    validateAudioNodeOptions(options);
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
      throw new InvalidAccessError(
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

    if (this.context !== destinationOrOutput.context) {
      throw new InvalidAccessError(
        'Source and destination are from different BaseAudioContexts'
      );
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
