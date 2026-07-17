import { InvalidAccessError } from '../errors';
import { IIIRFilterNode } from '../jsi-interfaces';
import AudioNode from './AudioNode';
import { IIRFilterOptions } from '../types';
import type BaseAudioContext from './BaseAudioContext';
import { validateIIRFilterOptions } from '../utils/validation';

export default class IIRFilterNode extends AudioNode {
  constructor(context: BaseAudioContext, options: IIRFilterOptions) {
    validateIIRFilterOptions(options);
    const iirFilterNode = context.context.createIIRFilter(options);
    super(context, iirFilterNode, options);
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
    (this.node as IIIRFilterNode).getFrequencyResponse(
      frequencyArray,
      magResponseOutput,
      phaseResponseOutput
    );
  }
}
