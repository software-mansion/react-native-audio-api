import { IRecorderAdapterNode } from '../interfaces';
import AudioNode from './AudioNode';

export default class RecorderAdapterNode extends AudioNode {
  /** @internal */
  public getNode(): IRecorderAdapterNode {
    return this.node as IRecorderAdapterNode;
  }
}
