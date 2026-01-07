import AudioNode from './AudioNode';
import BaseAudioContext from './BaseAudioContext';
import { InvalidStateError } from '../errors';
import { IWaveShaperNode } from '../interfaces';
import { WaveShaperOptions } from '../defaults';
import { TWaveShaperOptions } from '../types';

export default class WaveShaperNode extends AudioNode {
  private isCurveSet: boolean = false;

  constructor(context: BaseAudioContext, options?: TWaveShaperOptions) {
    const finalOptions: TWaveShaperOptions = {
      ...WaveShaperOptions,
      ...options,
    };

    const node = context.context.createWaveShaper(finalOptions);
    super(context, node);

    if (finalOptions.curve) {
      this.isCurveSet = true;
    }
  }

  get curve(): Float32Array | null {
    if (!this.isCurveSet) {
      return null;
    }

    return (this.node as IWaveShaperNode).curve;
  }

  get oversample(): OverSampleType {
    return (this.node as IWaveShaperNode).oversample;
  }

  set curve(curve: Float32Array | null) {
    if (curve !== null) {
      if (this.isCurveSet) {
        throw new InvalidStateError(
          'The curve can only be set once and cannot be changed afterwards.'
        );
      }

      if (curve.length < 2) {
        throw new InvalidStateError(
          'The curve must have at least two values if not null.'
        );
      }

      this.isCurveSet = true;
    }

    (this.node as IWaveShaperNode).setCurve(curve);
  }

  set oversample(value: OverSampleType) {
    (this.node as IWaveShaperNode).oversample = value;
  }
}
