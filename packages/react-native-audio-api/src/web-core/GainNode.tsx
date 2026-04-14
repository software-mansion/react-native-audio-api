import BaseAudioContext from './BaseAudioContext';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import { GainOptions } from '../types';

export default class GainNode extends AudioNode {
  readonly gain: AudioParam;

  constructor(context: BaseAudioContext, gainOptions?: GainOptions) {
    const gain = new globalThis.GainNode(context.context, gainOptions);
    super(context, gain);
    this.gain = new AudioParam(gain.gain, context);
  }
}
