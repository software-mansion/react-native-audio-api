import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';
import { DelayOptions } from '../types';

export default class DelayNode extends AudioNode {
  readonly delayTime: AudioParam;

  constructor(context: BaseAudioContext, options?: DelayOptions) {
    const delay = context.context.createDelay(options || {});
    super(context, delay);
    this.delayTime = new AudioParam(delay.delayTime, context);
  }
}
