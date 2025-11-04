import BaseAudioContext from './BaseAudioContext';
import AudioNode from './AudioNode';
import AudioBuffer from './AudioBuffer';

export default class ConvolverNode extends AudioNode {
  constructor(
    context: BaseAudioContext,
    node: globalThis.ConvolverNode,
    buffer: AudioBuffer | null = null,
    disableNormalization: boolean = false
  ) {
    super(context, node);

    (this.node as globalThis.ConvolverNode).normalize = !disableNormalization;
    if (buffer) {
      (this.node as globalThis.ConvolverNode).buffer = buffer.buffer;
    }
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
