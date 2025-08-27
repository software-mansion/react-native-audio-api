import AudioNode from '../core/AudioNode';
import { WindowType } from '../../types';
import { IndexSizeError } from '../../errors';

export default class AnalyserNode extends AudioNode {
  private static allowedFFTSize: number[] = [
    32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
  ];

  public get fftSize(): number {
    return (this.node as globalThis.AnalyserNode).fftSize;
  }

  public set fftSize(value: number) {
    if (!AnalyserNode.allowedFFTSize.includes(value)) {
      throw new IndexSizeError(
        `Provided value (${value}) must be a power of 2 between 32 and 32768`
      );
    }

    (this.node as globalThis.AnalyserNode).fftSize = value;
  }

  public get minDecibels(): number {
    return (this.node as globalThis.AnalyserNode).minDecibels;
  }

  public set minDecibels(value: number) {
    if (value >= this.maxDecibels) {
      throw new IndexSizeError(
        `The minDecibels value (${value}) must be less than maxDecibels`
      );
    }

    (this.node as globalThis.AnalyserNode).minDecibels = value;
  }

  public get maxDecibels(): number {
    return (this.node as globalThis.AnalyserNode).maxDecibels;
  }

  public set maxDecibels(value: number) {
    if (value <= this.minDecibels) {
      throw new IndexSizeError(
        `The maxDecibels value (${value}) must be greater than minDecibels`
      );
    }

    (this.node as globalThis.AnalyserNode).maxDecibels = value;
  }

  public get smoothingTimeConstant(): number {
    return (this.node as globalThis.AnalyserNode).smoothingTimeConstant;
  }

  public set smoothingTimeConstant(value: number) {
    if (value < 0 || value > 1) {
      throw new IndexSizeError(
        `The smoothingTimeConstant value (${value}) must be between 0 and 1`
      );
    }

    (this.node as globalThis.AnalyserNode).smoothingTimeConstant = value;
  }

  public get window(): WindowType {
    console.warn('React Native Audio API: window prop is not supported on web');
    return 'blackman';
  }

  public set window(value: WindowType) {
    console.warn('React Native Audio API: window prop is not supported on web');
  }

  public get frequencyBinCount(): number {
    return (this.node as globalThis.AnalyserNode).frequencyBinCount;
  }

  public getFloatFrequencyData(array: Float32Array<ArrayBuffer>): void {
    (this.node as globalThis.AnalyserNode).getFloatFrequencyData(array);
  }

  public getByteFrequencyData(array: Uint8Array<ArrayBuffer>): void {
    (this.node as globalThis.AnalyserNode).getByteFrequencyData(array);
  }

  public getFloatTimeDomainData(array: Float32Array<ArrayBuffer>): void {
    (this.node as globalThis.AnalyserNode).getFloatTimeDomainData(array);
  }

  public getByteTimeDomainData(array: Uint8Array<ArrayBuffer>): void {
    (this.node as globalThis.AnalyserNode).getByteTimeDomainData(array);
  }
}
