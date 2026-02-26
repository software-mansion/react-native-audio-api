import BaseAudioContext from './BaseAudioContext';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import { DelayOptions } from '../types';

export default class DelayNode extends AudioNode {
  readonly delayTime: AudioParam;

  constructor(context: BaseAudioContext, options?: DelayOptions) {
    const delay = new globalThis.DelayNode(context.context, options);
    super(context, delay);
    this.delayTime = new AudioParam(delay.delayTime, context);
  }
}
