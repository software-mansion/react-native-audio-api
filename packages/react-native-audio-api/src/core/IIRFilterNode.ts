import { NotSupportedError } from '../errors';
import { IIIRFilterNode } from '../interfaces';
import AudioNode from './AudioNode';
import { IIRFilterOptions } from '../types';
import BaseAudioContext from './BaseAudioContext';

export default class IIRFilterNode extends AudioNode {
  constructor(context: BaseAudioContext, options: IIRFilterOptions) {
    const iirFilterNode = context.context.createIIRFilter(options || {});
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
    (this.node as IIIRFilterNode).getFrequencyResponse(
      frequencyArray,
      magResponseOutput,
      phaseResponseOutput
    );
  }
}
