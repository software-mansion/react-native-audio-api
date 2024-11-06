import { InvalidAccessError } from '../errors';
import { IBiquadFilterNode } from '../interfaces';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import BaseAudioContext from './BaseAudioContext';
import { FilterType } from './types';

export default class BiquadFilterNode extends AudioNode {
  readonly frequency: AudioParam;
  readonly detune: AudioParam;
  readonly Q: AudioParam;
  readonly gain: AudioParam;

  constructor(context: BaseAudioContext, biquadFilter: IBiquadFilterNode) {
    super(context, biquadFilter);
    this.frequency = new AudioParam(biquadFilter.frequency);
    this.detune = new AudioParam(biquadFilter.detune);
    this.Q = new AudioParam(biquadFilter.Q);
    this.gain = new AudioParam(biquadFilter.gain);
  }

  public get type(): FilterType {
    return (this.node as IBiquadFilterNode).type;
  }

  public set type(value: FilterType) {
    (this.node as IBiquadFilterNode).type = value;
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
      throw new InvalidAccessError(
        `The lengths of the arrays are not the same frequencyArray: ${frequencyArray.length}, magResponseOutput: ${magResponseOutput.length}, phaseResponseOutput: ${phaseResponseOutput.length}`
      );
    }
    (this.node as IBiquadFilterNode).getFrequencyResponse(
      frequencyArray,
      magResponseOutput,
      phaseResponseOutput
    );
  }
}
