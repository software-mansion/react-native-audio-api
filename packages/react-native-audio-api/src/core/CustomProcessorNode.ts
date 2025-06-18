import { ICustomProcessorNode } from "../interfaces";
import { ProcessorMode } from "../types";
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

  public get processorMode(): ProcessorMode {
    return (this.node as ICustomProcessorNode).processorMode;
  }

  public set processorMode(value: ProcessorMode) {
    (this.node as ICustomProcessorNode).processorMode = value;
  }
}
