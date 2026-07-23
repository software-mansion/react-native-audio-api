import type BaseAudioContext from './BaseAudioContext';
import { IAnalyserNode } from '../jsi-interfaces';
import { AnalyserOptions } from '../types';
import AudioNode from './AudioNode';
import {
  AnalyserOptionsValidator,
  validateAnalyserFftSize,
  validateAnalyserMaxDecibels,
  validateAnalyserMinDecibels,
  validateAnalyserSmoothingTimeConstant,
} from '../utils/validation';
import { roundFromFloat32 } from '../utils/round';

export default class AnalyserNode extends AudioNode {
  constructor(context: BaseAudioContext, options?: AnalyserOptions) {
    AnalyserOptionsValidator.validate(options);
    const analyserNode: IAnalyserNode = context.context.createAnalyser(
      options || {}
    );
    super(context, analyserNode, options);
  }

  public get fftSize(): number {
    return (this.node as IAnalyserNode).fftSize;
  }

  public set fftSize(value: number) {
    validateAnalyserFftSize(value);
    (this.node as IAnalyserNode).fftSize = value;
  }

  public get minDecibels(): number {
    return roundFromFloat32((this.node as IAnalyserNode).minDecibels);
  }

  public set minDecibels(value: number) {
    validateAnalyserMinDecibels(value, this.maxDecibels);
    (this.node as IAnalyserNode).minDecibels = value;
  }

  public get maxDecibels(): number {
    return roundFromFloat32((this.node as IAnalyserNode).maxDecibels);
  }

  public set maxDecibels(value: number) {
    validateAnalyserMaxDecibels(value, this.minDecibels);
    (this.node as IAnalyserNode).maxDecibels = value;
  }

  public get smoothingTimeConstant(): number {
    return roundFromFloat32((this.node as IAnalyserNode).smoothingTimeConstant);
  }

  public set smoothingTimeConstant(value: number) {
    validateAnalyserSmoothingTimeConstant(value);
    (this.node as IAnalyserNode).smoothingTimeConstant = value;
  }

  public get frequencyBinCount(): number {
    return Math.floor((this.node as IAnalyserNode).fftSize / 2);
  }

  public getFloatFrequencyData(array: Float32Array): void {
    (this.node as IAnalyserNode).getFloatFrequencyData(array);
  }

  public getByteFrequencyData(array: Uint8Array): void {
    (this.node as IAnalyserNode).getByteFrequencyData(array);
  }

  public getFloatTimeDomainData(array: Float32Array): void {
    (this.node as IAnalyserNode).getFloatTimeDomainData(array);
  }

  public getByteTimeDomainData(array: Uint8Array): void {
    (this.node as IAnalyserNode).getByteTimeDomainData(array);
  }
}
