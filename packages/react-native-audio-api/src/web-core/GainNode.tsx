import BaseAudioContext from './BaseAudioContext';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';

export default class GainNode extends AudioNode {
  readonly gain: AudioParam;

  constructor(context: BaseAudioContext, gain: globalThis.GainNode) {
    super(context, gain);
    this.gain = new AudioParam(gain.gain, context);
  }
}
