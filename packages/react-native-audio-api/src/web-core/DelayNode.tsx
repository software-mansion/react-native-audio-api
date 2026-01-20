import BaseAudioContext from './BaseAudioContext';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import { TDelayOptions } from '../types';
import { DelayOptions } from '../defaults';

export default class DelayNode extends AudioNode {
  readonly delayTime: AudioParam;

  constructor(context: BaseAudioContext, options?: TDelayOptions) {
    const finalOptions = { ...DelayOptions, ...options };
    const delay = new globalThis.DelayNode(context.context, finalOptions);
    super(context, delay);
    this.delayTime = new AudioParam(delay.delayTime, context);
  }
}
