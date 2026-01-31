import { IGainNode } from '../interfaces';
import { TGainOptions } from '../types';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';

export default class GainNode extends AudioNode {
  readonly gain: AudioParam;

  constructor(context: BaseAudioContext, options?: TGainOptions) {
    const gainNode: IGainNode = context.context.createGain(options || {});
    super(context, gainNode);
    this.gain = new AudioParam(gainNode.gain, context);
  }
}
