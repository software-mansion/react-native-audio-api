import BaseAudioContext from './BaseAudioContext';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import { TGainOptions } from '../types';

export default class GainNode extends AudioNode {
  readonly gain: AudioParam;

  constructor(context: BaseAudioContext, gainOptions?: TGainOptions) {
    const gain = new globalThis.GainNode(context.context, gainOptions);
    super(context, gain);
    this.gain = new AudioParam(gain.gain, context);
  }
}
