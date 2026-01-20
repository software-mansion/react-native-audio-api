import { IGainNode } from '../interfaces';
import { GainOptions } from '../defaults';
import { TGainOptions } from '../types';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';

export default class GainNode extends AudioNode {
  readonly gain: AudioParam;

  constructor(context: BaseAudioContext, options?: TGainOptions) {
    const finalOptions: TGainOptions = {
      ...GainOptions,
      ...options,
    };
    const gainNode: IGainNode = context.context.createGain(finalOptions);
    super(context, gainNode);
    this.gain = new AudioParam(gainNode.gain, context);
  }
}
