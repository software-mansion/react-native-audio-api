import { IStereoPannerNode } from '../interfaces';
import AudioNode from '../core/AudioNode';
import AudioParam from '../core/AudioParam';
import BaseAudioContext from '../core/BaseAudioContext';

export default class StereoPannerNode extends AudioNode {
  readonly pan: AudioParam;

  constructor(context: BaseAudioContext, pan: IStereoPannerNode) {
    super(context, pan);
    this.pan = new AudioParam(pan.pan, context);
  }
}
