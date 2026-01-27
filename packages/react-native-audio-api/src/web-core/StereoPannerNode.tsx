import { TStereoPannerOptions } from '../types';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';

export default class StereoPannerNode extends AudioNode {
  readonly pan: AudioParam;

  constructor(
    context: BaseAudioContext,
    stereoPannerOptions?: TStereoPannerOptions
  ) {
    const pan = new globalThis.StereoPannerNode(
      context.context,
      stereoPannerOptions
    );
    super(context, pan);
    this.pan = new AudioParam(pan.pan, context);
  }
}
