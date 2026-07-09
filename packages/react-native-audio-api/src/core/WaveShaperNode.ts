import AudioNode from './AudioNode';
import type BaseAudioContext from './BaseAudioContext';
import { IWaveShaperNode } from '../jsi-interfaces';
import { WaveShaperOptions } from '../types';
import { validateWaveShaperCurve } from '../utils/validation';

export default class WaveShaperNode extends AudioNode {
  private isCurveSet: boolean = false;
  private _curve: Float32Array | null = null;

  constructor(context: BaseAudioContext, options?: WaveShaperOptions) {
    const node = context.context.createWaveShaper(options || {});
    super(context, node);
    if (options?.curve) {
      this._curve = options.curve;
      this.isCurveSet = true;
    }
  }

  get curve(): Float32Array | null {
    return this._curve;
  }

  get oversample(): OverSampleType {
    return (this.node as IWaveShaperNode).oversample;
  }

  set curve(curve: Float32Array | null) {
    validateWaveShaperCurve(curve, this.isCurveSet);

    if (curve !== null) {
      this.isCurveSet = true;
    }

    (this.node as IWaveShaperNode).setCurve(curve);
  }

  set oversample(value: OverSampleType) {
    (this.node as IWaveShaperNode).oversample = value;
  }
}
