import { InvalidStateError } from '../errors';
import AudioNode from './AudioNode';

export default class WaveShaperNode extends AudioNode {
  private isCurveSet: boolean = false;

  get curve(): Float32Array | null {
    if (!this.isCurveSet) {
      return null;
    }

    return (this.node as globalThis.WaveShaperNode).curve;
  }

  get oversample(): OverSampleType {
    return (this.node as globalThis.WaveShaperNode).oversample;
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

    (this.node as globalThis.WaveShaperNode).curve = curve;
  }

  set oversample(value: OverSampleType) {
    (this.node as globalThis.WaveShaperNode).oversample = value;
  }
}
