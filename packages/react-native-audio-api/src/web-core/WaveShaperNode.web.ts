import AudioNode from './AudioNode.web';
import BaseAudioContext from './BaseAudioContext.web';
import { WaveShaperOptions } from '../types';
import { toFloat32Array } from '../utils';
import { validateWaveShaperCurve } from '../utils/validation';

export default class WaveShaperNode extends AudioNode {
  private curveWasSet = false;
  private _curve: Float32Array | null = null;

  constructor(context: BaseAudioContext, options?: WaveShaperOptions) {
    super(context, new globalThis.WaveShaperNode(context.context, options));
    if (options?.curve) {
      this._curve = toFloat32Array(options.curve);
      this.curveWasSet = true;
    }
  }

  get curve(): Float32Array | null {
    return this._curve;
  }

  get oversample(): OverSampleType {
    return (this.node as globalThis.WaveShaperNode).oversample;
  }

  set curve(curve: Float32Array<ArrayBuffer> | null) {
    validateWaveShaperCurve(curve, this.curveWasSet);

    if (curve !== null) {
      this.curveWasSet = true;
    }

    this._curve = curve;
    (this.node as globalThis.WaveShaperNode).curve = curve;
  }

  set oversample(value: OverSampleType) {
    (this.node as globalThis.WaveShaperNode).oversample = value;
  }
}
