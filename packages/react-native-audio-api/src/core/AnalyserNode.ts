import type BaseAudioContext from './BaseAudioContext';
import { IndexSizeError } from '../errors';
import { IAnalyserNode } from '../jsi-interfaces';
import { AnalyserOptions } from '../types';
import AudioNode from './AudioNode';
import { AnalyserOptionsValidator } from '../options-validators';

const ANALYSER_ALLOWED_FFT_SIZE: number[] = [
  32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
] as const;

// The native engine stores these attributes as 32-bit floats, so reading them
// back loses the exact double the Web Audio spec expects (e.g. 0.8 ->
// 0.800000011920929). Rounding to float32's ~7 significant digits recovers it.
function roundFromFloat32(value: number): number {
  return Number(value.toPrecision(7));
}

export default class AnalyserNode extends AudioNode {
  constructor(context: BaseAudioContext, options?: AnalyserOptions) {
    AnalyserOptionsValidator.validate(options);
    const analyserNode: IAnalyserNode = context.context.createAnalyser(
      options || {}
    );
    super(context, analyserNode);
  }

  public get fftSize(): number {
    return (this.node as IAnalyserNode).fftSize;
  }

  public set fftSize(value: number) {
    if (!ANALYSER_ALLOWED_FFT_SIZE.includes(value)) {
      throw new IndexSizeError(
        `Provided value (${value}) must be a power of 2 between 32 and 32768`
      );
    }

    (this.node as IAnalyserNode).fftSize = value;
  }

  public get minDecibels(): number {
    return roundFromFloat32((this.node as IAnalyserNode).minDecibels);
  }

  public set minDecibels(value: number) {
    if (value >= this.maxDecibels) {
      throw new IndexSizeError(
        `The minDecibels value (${value}) must be less than maxDecibels`
      );
    }

    (this.node as IAnalyserNode).minDecibels = value;
  }

  public get maxDecibels(): number {
    return roundFromFloat32((this.node as IAnalyserNode).maxDecibels);
  }

  public set maxDecibels(value: number) {
    if (value <= this.minDecibels) {
      throw new IndexSizeError(
        `The maxDecibels value (${value}) must be greater than minDecibels`
      );
    }

    (this.node as IAnalyserNode).maxDecibels = value;
  }

  public get smoothingTimeConstant(): number {
    return roundFromFloat32((this.node as IAnalyserNode).smoothingTimeConstant);
  }

  public set smoothingTimeConstant(value: number) {
    if (value < 0 || value > 1) {
      throw new IndexSizeError(
        `The smoothingTimeConstant value (${value}) must be between 0 and 1`
      );
    }

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
