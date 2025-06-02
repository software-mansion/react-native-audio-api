import BaseAudioContext from './BaseAudioContext';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';

export default class StereoPannerNode extends AudioNode {
  readonly pan: AudioParam;

  constructor(context: BaseAudioContext, pan: globalThis.StereoPannerNode) {
    super(context, pan);
    this.pan = new AudioParam(pan.pan, context);
  }
}
