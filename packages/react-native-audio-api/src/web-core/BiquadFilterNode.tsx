import AudioParam from './AudioParam';
import AudioNode from './AudioNode';
import BaseAudioContext from './BaseAudioContext';
import { BiquadFilterType } from '../types';
import { InvalidAccessError } from '../errors';

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
    this.frequency = new AudioParam(biquadFilter.frequency, context);
    this.detune = new AudioParam(biquadFilter.detune, context);
    this.Q = new AudioParam(biquadFilter.Q, context);
    this.gain = new AudioParam(biquadFilter.gain, context);
  }

  public get type(): BiquadFilterType {
    return (this.node as globalThis.BiquadFilterNode).type;
  }

  public set type(value: BiquadFilterType) {
    (this.node as globalThis.BiquadFilterNode).type = value;
  }

  public getFrequencyResponse(
    frequencyArray: Float32Array,
    magResponseOutput: Float32Array,
    phaseResponseOutput: Float32Array
  ) {
    if (
      frequencyArray.length !== magResponseOutput.length ||
      frequencyArray.length !== phaseResponseOutput.length
    ) {
      throw new InvalidAccessError(
        `The lengths of the arrays are not the same frequencyArray: ${frequencyArray.length}, magResponseOutput: ${magResponseOutput.length}, phaseResponseOutput: ${phaseResponseOutput.length}`
      );
    }

    (this.node as globalThis.BiquadFilterNode).getFrequencyResponse(
      frequencyArray,
      magResponseOutput,
      phaseResponseOutput
    );
  }
}
