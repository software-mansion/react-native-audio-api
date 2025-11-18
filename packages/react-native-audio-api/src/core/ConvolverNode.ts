import { IConvolverNode } from '../interfaces';
import BaseAudioContext from './BaseAudioContext';
import AudioNode from './AudioNode';
import AudioBuffer from './AudioBuffer';

export default class ConvolverNode extends AudioNode {
  constructor(context: BaseAudioContext, node: IConvolverNode) {
    super(context, node);
    this.normalize = node.normalize;
  }

  public get buffer(): AudioBuffer | null {
    const buffer = (this.node as IConvolverNode).buffer;
    if (!buffer) {
      return null;
    }
    return new AudioBuffer(buffer);
  }

  public set buffer(buffer: AudioBuffer | null) {
    if (!buffer) {
      (this.node as IConvolverNode).setBuffer(null);
      return;
    }
    (this.node as IConvolverNode).setBuffer(buffer.buffer);
  }

  public get normalize(): boolean {
    return (this.node as IConvolverNode).normalize;
  }

  public set normalize(value: boolean) {
    (this.node as IConvolverNode).normalize = value;
  }
}
