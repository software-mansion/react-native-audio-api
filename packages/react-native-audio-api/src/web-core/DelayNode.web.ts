import BaseAudioContext from './BaseAudioContext.web';
import AudioNode from './AudioNode.web';
import AudioParam from './AudioParam.web';
import { DelayOptions } from '../types';

export default class DelayNode extends AudioNode {
  readonly delayTime: AudioParam;

  constructor(context: BaseAudioContext, options?: DelayOptions) {
    const delay = new globalThis.DelayNode(context.context, options);
    super(context, delay);
    this.delayTime = new AudioParam(delay.delayTime, context);
  }
}
