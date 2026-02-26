import { IConstantSourceNode } from '../interfaces';
import { ConstantSourceOptions } from '../types';
import AudioParam from './AudioParam';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import BaseAudioContext from './BaseAudioContext';

export default class ConstantSourceNode extends AudioScheduledSourceNode {
  readonly offset: AudioParam;

  constructor(context: BaseAudioContext, options?: ConstantSourceOptions) {
    const node: IConstantSourceNode = context.context.createConstantSource(
      options || {}
    );
    super(context, node);
    this.offset = new AudioParam(node.offset, context);
  }
}
