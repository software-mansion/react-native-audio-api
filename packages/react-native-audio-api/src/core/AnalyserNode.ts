import { IndexSizeError } from '../errors';
import { IAnalyserNode } from '../interfaces';
import AudioNode from './AudioNode';
import BaseAudioContext from './BaseAudioContext';

export default class AnalyserNode extends AudioNode {
  readonly frequencyBinCount: number;
  private static allowedFFTSize: number[] = [
    32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
  ];

  constructor(context: BaseAudioContext, analyser: IAnalyserNode) {
    super(context, analyser);

    this.fftSize = analyser.fftSize;
    this.frequencyBinCount = analyser.frequencyBinCount;
    this.minDecibels = analyser.minDecibels;
    this.maxDecibels = analyser.maxDecibels;
    this.smoothingTimeConstant = analyser.smoothingTimeConstant;
  }

  public get fftSize(): number {
    return (this.node as IAnalyserNode).fftSize;
  }

  public set fftSize(value: number) {
    if (!AnalyserNode.allowedFFTSize.includes(value)) {
      throw new IndexSizeError(
        `Provided value (${value}) must be a power of 2 2^5 and 2^15`
      );
    }

    (this.node as IAnalyserNode).fftSize = value;
  }

  public get minDecibels(): number {
    return (this.node as IAnalyserNode).minDecibels;
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
    return (this.node as IAnalyserNode).maxDecibels;
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
    return (this.node as IAnalyserNode).smoothingTimeConstant;
  }

  public set smoothingTimeConstant(value: number) {
    if (value < 0 || value > 1) {
      throw new IndexSizeError(
        `The smoothingTimeConstant value (${value}) must be between 0 and 1`
      );
    }

    (this.node as IAnalyserNode).smoothingTimeConstant = value;
  }

  public getFloatFrequencyData(array: number[]): void {
    (this.node as IAnalyserNode).getFloatFrequencyData(array);
  }

  public getByteFrequencyData(array: number[]): void {
    (this.node as IAnalyserNode).getByteFrequencyData(array);
  }

  public getFloatTimeDomainData(array: number[]): void {
    (this.node as IAnalyserNode).getFloatTimeDomainData(array);
  }

  public getByteTimeDomainData(array: number[]): void {
    (this.node as IAnalyserNode).getByteTimeDomainData(array);
  }
}
