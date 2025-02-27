import { IStretcherNode } from '../interfaces';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';

export default class StretcherNode extends AudioNode {
  readonly rate: AudioParam;
  readonly semitones: AudioParam;

  constructor(context: BaseAudioContext, stretcher: IStretcherNode) {
    super(context, stretcher);
    this.rate = new AudioParam(stretcher.rate);
    this.semitones = new AudioParam(stretcher.semitones);
  }
}
