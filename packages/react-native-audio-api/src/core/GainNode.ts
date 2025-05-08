import { IGainNode } from '../interfaces';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';

export default class GainNode extends AudioNode {
  readonly gain: AudioParam;

  constructor(context: BaseAudioContext, gain: IGainNode) {
    super(context, gain);
    this.gain = new AudioParam(gain.gain, context);
  }
}
