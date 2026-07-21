import AudioNode from './AudioNode';
import type BaseAudioContext from './BaseAudioContext';
import { IWaveShaperNode } from '../jsi-interfaces';
import { WaveShaperOptions } from '../types';
import { toFloat32Array } from '../utils';
import { validateWaveShaperCurve } from '../utils/validation';

export default class WaveShaperNode extends AudioNode {
  private curveWasSet = false;
  private _curve: Float32Array | null = null;

  constructor(context: BaseAudioContext, options?: WaveShaperOptions) {
    const node = context.context.createWaveShaper(options || {});
    super(context, node);
    if (options?.curve) {
      this._curve = toFloat32Array(options.curve);
      this.curveWasSet = true;
    }
  }

  get curve(): Float32Array | null {
    return this._curve;
  }

  get oversample(): OverSampleType {
    return (this.node as IWaveShaperNode).oversample;
  }

  set curve(curve: Float32Array | null) {
    validateWaveShaperCurve(curve, this.curveWasSet);

    if (curve !== null) {
      this.curveWasSet = true;
    }

    this._curve = curve;
    (this.node as IWaveShaperNode).setCurve(curve);
  }

  set oversample(value: OverSampleType) {
    (this.node as IWaveShaperNode).oversample = value;
  }
}
