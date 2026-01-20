import BaseAudioContext from './BaseAudioContext';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import { TSteroPannerOptions } from '../types';

export default class StereoPannerNode extends AudioNode {
  readonly pan: AudioParam;

  constructor(
    context: BaseAudioContext,
    stereoPannerOptions?: TSteroPannerOptions
  ) {
    const pan = new globalThis.StereoPannerNode(
      context.context,
      stereoPannerOptions
    );
    super(context, pan);
    this.pan = new AudioParam(pan.pan, context);
  }
}
