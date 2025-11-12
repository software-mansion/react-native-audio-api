import BaseAudioContext from './BaseAudioContext';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';

export default class DelayNode extends AudioNode {
  readonly delayTime: AudioParam;

  constructor(context: BaseAudioContext, delay: globalThis.DelayNode) {
    super(context, delay);
    this.delayTime = new AudioParam(delay.delayTime, context);
  }
}
