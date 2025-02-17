import AudioParam from './AudioParam';
import AudioNode from './AudioNode';
import BaseAudioContext from './BaseAudioContext';
import { BiquadFilterType } from '../index.native';

export default class BiquadFilterNode extends AudioNode {
  readonly frequency: AudioParam;
  readonly detune: AudioParam;
  readonly Q: AudioParam;
  readonly gain: AudioParam;

  constructor(
    context: BaseAudioContext,
    biquadFilter: globalThis.BiquadFilterNode
  ) {
    super(context, biquadFilter);
    this.frequency = new AudioParam(biquadFilter.frequency);
    this.detune = new AudioParam(biquadFilter.detune);
    this.Q = new AudioParam(biquadFilter.Q);
    this.gain = new AudioParam(biquadFilter.gain);
  }

  public get type(): BiquadFilterType {
    return (this.node as globalThis.BiquadFilterNode).type;
  }

  public set type(value: BiquadFilterType) {
    (this.node as globalThis.BiquadFilterNode).type = value;
  }

  public getFrequencyResponse(
    frequencyArray: number[],
    magResponseOutput: number[],
    phaseResponseOutput: number[]
  ) {
    if (
      frequencyArray.length !== magResponseOutput.length ||
      frequencyArray.length !== phaseResponseOutput.length
    ) {
      throw new Error(
        `The lengths of the arrays are not the same frequencyArray: ${frequencyArray.length}, magResponseOutput: ${magResponseOutput.length}, phaseResponseOutput: ${phaseResponseOutput.length}`
      );
    }
    const magData = new Float32Array(magResponseOutput);
    const phaseData = new Float32Array(phaseResponseOutput);

    (this.node as globalThis.BiquadFilterNode).getFrequencyResponse(
      new Float32Array(frequencyArray),
      magData,
      phaseData
    );

    for (let i = 0; i < magData.length; i++) {
      magResponseOutput[i] = magData[i];
    }

    for (let i = 0; i < phaseData.length; i++) {
      phaseResponseOutput[i] = phaseData[i];
    }
  }
}
