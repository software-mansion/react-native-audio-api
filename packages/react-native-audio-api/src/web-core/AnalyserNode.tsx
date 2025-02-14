import { AudioNode } from './AudioNode';
import { WindowType } from '../index.native';

export class AnalyserNode extends AudioNode {
  fftSize: number;
  readonly frequencyBinCount: number;
  minDecibels: number;
  maxDecibels: number;
  smoothingTimeConstant: number;

  constructor(context: AudioContext, node: globalThis.AnalyserNode) {
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

  public getByteFrequencyData(array: number[]): void {
    const data = new Uint8Array(array);

    (this.node as globalThis.AnalyserNode).getByteFrequencyData(data);

    for (let i = 0; i < array.length; i++) {
      array[i] = data[i];
    }
  }

  public getByteTimeDomainData(array: number[]): void {
    const data = new Uint8Array(array);

    (this.node as globalThis.AnalyserNode).getByteTimeDomainData(data);

    for (let i = 0; i < array.length; i++) {
      array[i] = data[i];
    }
  }

  public getFloatFrequencyData(array: number[]): void {
    const data = new Float32Array(array);
    (this.node as globalThis.AnalyserNode).getFloatFrequencyData(data);

    for (let i = 0; i < array.length; i++) {
      array[i] = data[i];
    }
  }

  public getFloatTimeDomainData(array: number[]): void {
    const data = new Float32Array(array);
    (this.node as globalThis.AnalyserNode).getFloatTimeDomainData(data);

    for (let i = 0; i < array.length; i++) {
      array[i] = data[i];
    }
  }
}
