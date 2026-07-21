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

  public connect(
    destination: AudioNode | AudioParam,
    output: number = 0,
    input: number = 0
  ): AudioNode | AudioParam {
    if (this.context !== destination.context) {
      throw new Error(
        'Source and destination are from different BaseAudioContexts'
      );
    }

    if (destination instanceof AudioParam) {
      this.node.connect(destination.param, output);
    } else {
      this.node.connect(destination.node, output, input);
    }

    return destination;
  }

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
      this.node.disconnect(destinationOrOutput);
      return;
    }

    if (destinationOrOutput instanceof AudioParam) {
      if (output === undefined) {
        this.node.disconnect(destinationOrOutput.param);
      } else {
        this.node.disconnect(destinationOrOutput.param, output);
      }
      return;
    }

    if (output === undefined) {
      this.node.disconnect(destinationOrOutput.node);
    } else if (input === undefined) {
      this.node.disconnect(destinationOrOutput.node, output);
    } else {
      this.node.disconnect(destinationOrOutput.node, output, input);
    }
  }
}
