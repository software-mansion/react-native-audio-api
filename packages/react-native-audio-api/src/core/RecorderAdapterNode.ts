import { IRecorderAdapterNode } from '../interfaces';
import AudioNode from './AudioNode';
import BaseAudioContext from './BaseAudioContext';

export default class RecorderAdapterNode extends AudioNode {
  /** @internal */
  public wasConnected: boolean = false;

  constructor(context: BaseAudioContext) {
    super(context, context.context.createRecorderAdapter());
  }

  /** @internal */
  public getNode(): IRecorderAdapterNode {
    return this.node as IRecorderAdapterNode;
  }
}
