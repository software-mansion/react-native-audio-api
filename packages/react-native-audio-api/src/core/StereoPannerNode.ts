import { IStereoPannerNode } from '../interfaces';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';

export default class StereoPannerNode extends AudioNode {
  readonly pan: AudioParam;

  constructor(context: BaseAudioContext, pan: IStereoPannerNode) {
    super(context, pan);
    this.pan = new AudioParam(pan.pan, context);
  }
}
