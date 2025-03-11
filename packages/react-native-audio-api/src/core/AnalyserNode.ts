import { IndexSizeError } from '../errors';
import { IAnalyserNode } from '../interfaces';
import { WindowType } from '../types';
import AudioNode from './AudioNode';

export default class AnalyserNode extends AudioNode {
  private static allowedFFTSize: number[] = [
    32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
  ];

  public get fftSize(): number {
    return (this.node as IAnalyserNode).fftSize;
  }

  public set fftSize(value: number) {
    if (!AnalyserNode.allowedFFTSize.includes(value)) {
      throw new IndexSizeError(
        `Provided value (${value}) must be a power of 2 between 32 and 32768`
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

  public get window(): WindowType {
    return (this.node as IAnalyserNode).window;
  }

  public set window(value: WindowType) {
    (this.node as IAnalyserNode).window = value;
  }

  public get frequencyBinCount(): number {
    return (this.node as IAnalyserNode).frequencyBinCount;
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
