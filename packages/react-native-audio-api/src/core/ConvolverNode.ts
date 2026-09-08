import { IConvolverNode } from '../jsi-interfaces';
import { ConvolverOptions } from '../types';
import type BaseAudioContext from './BaseAudioContext';
import AudioNode from './AudioNode';
import AudioBuffer from './AudioBuffer';
import {
  ConvolverOptionsValidator,
  validateConvolverBufferChannelCount,
  validateConvolverBufferSampleRate,
} from '../utils/validation';

export default class ConvolverNode extends AudioNode {
  private _buffer: AudioBuffer | null = null;

  constructor(context: BaseAudioContext, options?: ConvolverOptions) {
    ConvolverOptionsValidator.validate(options);
    const convolverNode: IConvolverNode = context.context.createConvolver(
      options || {}
    );
    super(context, convolverNode, options);

    if (options?.buffer) {
      this.buffer = options.buffer as AudioBuffer;
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

    // Spec setter steps: the engine has no guard of its own, so an impulse
    // response the convolution matrix is undefined for must be rejected here.
    validateConvolverBufferChannelCount(buffer.numberOfChannels);
    validateConvolverBufferSampleRate(
      buffer.sampleRate,
      this.context.sampleRate
    );

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
