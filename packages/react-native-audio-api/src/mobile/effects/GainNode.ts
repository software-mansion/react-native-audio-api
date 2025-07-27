import { IGainNode } from '../interfaces';
import AudioNode from '../core/AudioNode';
import AudioParam from '../core/AudioParam';
import BaseAudioContext from '../core/BaseAudioContext';

export default class GainNode extends AudioNode {
  readonly gain: AudioParam;

  constructor(context: BaseAudioContext, gain: IGainNode) {
    super(context, gain);
    this.gain = new AudioParam(gain.gain, context);
  }
}
