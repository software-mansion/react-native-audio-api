import BaseAudioContext from './BaseAudioContext';
import AudioNode from './AudioNode';
import AudioBuffer from './AudioBuffer';
import { ConvolverOptions } from '../types';

export default class ConvolverNode extends AudioNode {
  constructor(context: BaseAudioContext, convolverOptions?: ConvolverOptions) {
    const convolver = new globalThis.ConvolverNode(context.context, {
      ...convolverOptions,
      ...(convolverOptions?.buffer
        ? { buffer: convolverOptions.buffer.buffer }
        : {}),
    });

    const node = convolver;
    super(context, node);
  }

  public get buffer(): AudioBuffer | null {
    const buffer = (this.node as globalThis.ConvolverNode).buffer;
    if (!buffer) {
      return null;
    }
    return new AudioBuffer(buffer);
  }

  public set buffer(buffer: AudioBuffer | null) {
    if (!buffer) {
      (this.node as globalThis.ConvolverNode).buffer = null;
    } else {
      (this.node as globalThis.ConvolverNode).buffer = buffer.buffer;
    }
  }

  public get normalize(): boolean {
    return (this.node as globalThis.ConvolverNode).normalize;
  }

  public set normalize(value: boolean) {
    (this.node as globalThis.ConvolverNode).normalize = value;
  }
}
