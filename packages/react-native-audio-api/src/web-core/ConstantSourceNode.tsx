import AudioParam from './AudioParam';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import BaseAudioContext from './BaseAudioContext';
import { TConstantSourceOptions } from '../types';

export default class ConstantSourceNode extends AudioScheduledSourceNode {
  readonly offset: AudioParam;

  constructor(context: BaseAudioContext, options?: TConstantSourceOptions) {
    const node = new globalThis.ConstantSourceNode(context.context, options);
    super(context, node);
    this.offset = new AudioParam(node.offset, context);
  }
}
