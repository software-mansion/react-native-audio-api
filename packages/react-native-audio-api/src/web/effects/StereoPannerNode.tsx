import AudioParam from '../core/AudioParam';
import AudioNode from '../core/AudioNode';
import BaseAudioContext from '../core/BaseAudioContext';

export default class StereoPannerNode extends AudioNode {
  readonly pan: AudioParam;

  constructor(context: BaseAudioContext, pan: globalThis.StereoPannerNode) {
    super(context, pan);
    this.pan = new AudioParam(pan.pan, context);
  }
}
