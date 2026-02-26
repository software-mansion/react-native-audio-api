import AudioNode from './AudioNode';
import { WindowType, AnalyserOptions } from '../types';
import BaseAudioContext from './BaseAudioContext';

export default class AnalyserNode extends AudioNode {
  fftSize: number;
  readonly frequencyBinCount: number;
  minDecibels: number;
  maxDecibels: number;
  smoothingTimeConstant: number;

  constructor(context: BaseAudioContext, analyserOptions?: AnalyserOptions) {
    const node = new globalThis.AnalyserNode(context.context, analyserOptions);
    super(context, node);

    this.fftSize = node.fftSize;
    this.frequencyBinCount = node.frequencyBinCount;
    this.minDecibels = node.minDecibels;
    this.maxDecibels = node.maxDecibels;
    this.smoothingTimeConstant = node.smoothingTimeConstant;
  }

  public get window(): WindowType {
    return 'blackman';
  }

  public set window(value: WindowType) {
    console.log(
      'React Native Audio API: setting window is not supported on web'
    );
  }

  public getByteFrequencyData(array: Uint8Array): void {
    (this.node as globalThis.AnalyserNode).getByteFrequencyData(
      array as Uint8Array<ArrayBuffer>
    );
  }

  public getByteTimeDomainData(array: Uint8Array): void {
    (this.node as globalThis.AnalyserNode).getByteTimeDomainData(
      array as Uint8Array<ArrayBuffer>
    );
  }

  public getFloatFrequencyData(array: Float32Array): void {
    (this.node as globalThis.AnalyserNode).getFloatFrequencyData(
      array as Float32Array<ArrayBuffer>
    );
  }

  public getFloatTimeDomainData(array: Float32Array): void {
    (this.node as globalThis.AnalyserNode).getFloatTimeDomainData(
      array as Float32Array<ArrayBuffer>
    );
  }
}
