import AudioNode from '../core/AudioNode';
import BaseAudioContext from '../core/BaseAudioContext';
import { WindowType } from '../../types';

export default class AnalyserNode extends AudioNode {
  fftSize: number;
  readonly frequencyBinCount: number;
  minDecibels: number;
  maxDecibels: number;
  smoothingTimeConstant: number;

  constructor(context: BaseAudioContext, node: globalThis.AnalyserNode) {
    super(context, node);

    this.fftSize = node.fftSize;
    this.frequencyBinCount = node.frequencyBinCount;
    this.minDecibels = node.minDecibels;
    this.maxDecibels = node.maxDecibels;
    this.smoothingTimeConstant = node.smoothingTimeConstant;
  }

  public get window(): WindowType {
    console.warn('React Native Audio API: window prop is not supported on web');
    return 'blackman';
  }

  public set window(value: WindowType) {
    console.warn('React Native Audio API: window prop is not supported on web');
  }

  public getByteFrequencyData(array: Uint8Array): void {
    (this.node as globalThis.AnalyserNode).getByteFrequencyData(array);
  }

  public getByteTimeDomainData(array: Uint8Array): void {
    (this.node as globalThis.AnalyserNode).getByteTimeDomainData(array);
  }

  public getFloatFrequencyData(array: Float32Array): void {
    (this.node as globalThis.AnalyserNode).getFloatFrequencyData(array);
  }

  public getFloatTimeDomainData(array: Float32Array): void {
    (this.node as globalThis.AnalyserNode).getFloatTimeDomainData(array);
  }
}
