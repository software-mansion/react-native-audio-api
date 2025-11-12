import { IDelayNode } from '../interfaces';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';

export default class DelayNode extends AudioNode {
  readonly delayTime: AudioParam;

  constructor(context: BaseAudioContext, delay: IDelayNode) {
    super(context, delay);
    this.delayTime = new AudioParam(delay.delayTime, context);
  }
}
