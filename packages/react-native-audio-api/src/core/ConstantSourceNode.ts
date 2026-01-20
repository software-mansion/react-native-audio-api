import { IConstantSourceNode } from '../interfaces';
import { ConstantSourceOptions } from '../defaults';
import { TConstantSourceOptions } from '../types';
import AudioParam from './AudioParam';
import AudioScheduledSourceNode from './AudioScheduledSourceNode';
import BaseAudioContext from './BaseAudioContext';

export default class ConstantSourceNode extends AudioScheduledSourceNode {
  readonly offset: AudioParam;

  constructor(context: BaseAudioContext, options?: TConstantSourceOptions) {
    const finalOptions: TConstantSourceOptions = {
      ...ConstantSourceOptions,
      ...options,
    };
    const node: IConstantSourceNode =
      context.context.createConstantSource(finalOptions);
    super(context, node);
    this.offset = new AudioParam(node.offset, context);
  }
}
