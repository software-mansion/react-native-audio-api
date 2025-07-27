import AudioParam from '../core/AudioParam';
import AudioNode from '../core/AudioNode';
import BaseAudioContext from '../core/BaseAudioContext';

export default class GainNode extends AudioNode {
  readonly gain: AudioParam;

  constructor(context: BaseAudioContext, gain: globalThis.GainNode) {
    super(context, gain);
    this.gain = new AudioParam(gain.gain, context);
  }
}
