import { IStereoPannerNode } from '../interfaces';
import { StereoPannerOptions } from '../types';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';

export default class StereoPannerNode extends AudioNode {
  readonly pan: AudioParam;

  constructor(context: BaseAudioContext, options?: StereoPannerOptions) {
    const pan: IStereoPannerNode = context.context.createStereoPanner(
      options || {}
    );
    super(context, pan);
    this.pan = new AudioParam(pan.pan, context);
  }
}
