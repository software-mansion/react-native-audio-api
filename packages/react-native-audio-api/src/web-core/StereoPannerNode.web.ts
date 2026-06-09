import { StereoPannerOptions } from '../types';
import AudioNode from './AudioNode.web';
import AudioParam from './AudioParam.web';
import BaseAudioContext from './BaseAudioContext.web';

export default class StereoPannerNode extends AudioNode {
  readonly pan: AudioParam;

  constructor(
    context: BaseAudioContext,
    stereoPannerOptions?: StereoPannerOptions
  ) {
    const pan = new globalThis.StereoPannerNode(
      context.context,
      stereoPannerOptions
    );
    super(context, pan);
    this.pan = new AudioParam(pan.pan, context);
  }
}
