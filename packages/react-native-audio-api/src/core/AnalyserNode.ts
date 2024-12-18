import { IAnalyserNode } from '../interfaces';
import AudioNode from './AudioNode';
import BaseAudioContext from './BaseAudioContext';

export default class AnalyserNode extends AudioNode {
  fftSize: number;
  readonly frequencyBinCount: number;
  minDecibels: number;
  maxDecibels: number;
  smoothingTimeConstant: number;

  constructor(context: BaseAudioContext, analyser: IAnalyserNode) {
    super(context, analyser);

    this.fftSize = analyser.fftSize;
    this.frequencyBinCount = analyser.frequencyBinCount;
    this.minDecibels = analyser.minDecibels;
    this.maxDecibels = analyser.maxDecibels;
    this.smoothingTimeConstant = analyser.smoothingTimeConstant;
  }

  getFloatFrequencyData(array: number[]): void {
    (this.node as IAnalyserNode).getFloatFrequencyData(array);
  }

  getByteFrequencyData(array: number[]): void {
    (this.node as IAnalyserNode).getByteFrequencyData(array);
  }

  getFloatTimeDomainData(array: number[]): void {
    (this.node as IAnalyserNode).getFloatTimeDomainData(array);
  }

  getByteTimeDomainData(array: number[]): void {
    (this.node as IAnalyserNode).getByteTimeDomainData(array);
  }
}
