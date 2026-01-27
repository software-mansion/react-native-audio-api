import { StereoPannerOptions } from '../defaults';
import { IStereoPannerNode } from '../interfaces';
import { TStereoPannerOptions } from '../types';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';

export default class StereoPannerNode extends AudioNode {
  readonly pan: AudioParam;

  constructor(context: BaseAudioContext, options?: TStereoPannerOptions) {
    const finalOptions: TStereoPannerOptions = {
      ...StereoPannerOptions,
      ...options,
    };
    const pan: IStereoPannerNode =
      context.context.createStereoPanner(finalOptions);
    super(context, pan);
    this.pan = new AudioParam(pan.pan, context);
  }
}
