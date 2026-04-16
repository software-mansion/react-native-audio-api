import { IConvolverNode } from '../interfaces';
import { ConvolverOptions } from '../types';
import type BaseAudioContext from './BaseAudioContext';
import AudioNode from './AudioNode';
import AudioBuffer from './AudioBuffer';

export default class ConvolverNode extends AudioNode {
  private _buffer: AudioBuffer | null = null;

  constructor(context: BaseAudioContext, options?: ConvolverOptions) {
    const convolverNode: IConvolverNode = context.context.createConvolver(
      options || {}
    );
    super(context, convolverNode);

    if (options?.buffer) {
      this.buffer = options.buffer;
    }
    this.normalize = convolverNode.normalize;
  }

  public get buffer(): AudioBuffer | null {
    return this._buffer;
  }

  public set buffer(buffer: AudioBuffer | null) {
    if (!buffer) {
      (this.node as IConvolverNode).setBuffer(null);
      this._buffer = null;
      return;
    }
    (this.node as IConvolverNode).setBuffer(buffer.buffer);
    this._buffer = buffer;
  }

  public get normalize(): boolean {
    return (this.node as IConvolverNode).normalize;
  }

  public set normalize(value: boolean) {
    (this.node as IConvolverNode).normalize = value;
  }
}
