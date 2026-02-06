import { NotSupportedError } from '../errors';
import AudioNode from './AudioNode';
import { TIIRFilterOptions } from '../types';
import BaseAudioContext from './BaseAudioContext';

export default class IIRFilterNode extends AudioNode {
  constructor(context: BaseAudioContext, options: TIIRFilterOptions) {
    const iirFilterNode = new globalThis.IIRFilterNode(
      context.context,
      options
    );
    super(context, iirFilterNode);
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
      throw new NotSupportedError(
        `The lengths of the arrays are not the same frequencyArray: ${frequencyArray.length}, magResponseOutput: ${magResponseOutput.length}, phaseResponseOutput: ${phaseResponseOutput.length}`
      );
    }

    (this.node as globalThis.IIRFilterNode).getFrequencyResponse(
      frequencyArray,
      magResponseOutput,
      phaseResponseOutput
    );
  }
}
