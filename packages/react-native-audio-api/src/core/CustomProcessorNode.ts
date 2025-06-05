import { ICustomProcessorNode } from "../interfaces";
import { UUID } from "../types";
import AudioNode from "./AudioNode";
import AudioParam from "./AudioParam";
import BaseAudioContext from "./BaseAudioContext";

export default class CustomProcessorNode extends AudioNode {
  readonly customProcessor: AudioParam;

  constructor(
    context: BaseAudioContext,
    customProcessor: ICustomProcessorNode
  ) {
    super(context, customProcessor);
    this.customProcessor = new AudioParam(
      customProcessor.customProcessor,
      context
    );
  }

  public get identifier(): UUID {
    return (this.node as ICustomProcessorNode).identifier;
  }

  public set identifier(value: UUID) {
    (this.node as ICustomProcessorNode).identifier = value;
  }
}