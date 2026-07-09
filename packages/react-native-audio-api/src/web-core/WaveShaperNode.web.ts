import AudioNode from './AudioNode.web';
import { validateWaveShaperCurve } from '../utils/validation';

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

  set curve(curve: Float32Array<ArrayBuffer> | null) {
    validateWaveShaperCurve(curve, this.isCurveSet);

    if (curve !== null) {
      this.isCurveSet = true;
    }

    (this.node as globalThis.WaveShaperNode).curve = curve;
  }

  set oversample(value: OverSampleType) {
    (this.node as globalThis.WaveShaperNode).oversample = value;
  }
}
