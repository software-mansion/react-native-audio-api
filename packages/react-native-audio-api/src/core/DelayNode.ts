import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';
import { TDelayOptions } from '../types';
import { DelayOptions } from '../defaults';

export default class DelayNode extends AudioNode {
  readonly delayTime: AudioParam;

  constructor(context: BaseAudioContext, options?: TDelayOptions) {
    const finalOptions = { ...DelayOptions, ...options };
    const delay = context.context.createDelay(finalOptions);
    super(context, delay);
    this.delayTime = new AudioParam(delay.delayTime, context);
  }
}
