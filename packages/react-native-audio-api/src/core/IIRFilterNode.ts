import {
  InvalidAccessError,
  InvalidStateError,
  NotSupportedError,
} from '../errors';
import { IIIRFilterNode } from '../interfaces';
import AudioNode from './AudioNode';
import { IIRFilterOptions } from '../types';
import BaseAudioContext from './BaseAudioContext';

export default class IIRFilterNode extends AudioNode {
  constructor(context: BaseAudioContext, options: IIRFilterOptions) {
    IIRFilterNode.validateIIRFilterOptions(options);
    const iirFilterNode = context.context.createIIRFilter(options);
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

  private static validateIIRFilterOptions(options: IIRFilterOptions) {
    const { feedforward, feedback } = options;

    if (feedforward.length === 0 || feedforward.length > 20) {
      throw new NotSupportedError(
        `The length of feedforward must be between 1 and 20, but got ${feedforward.length}`
      );
    }

    if (feedforward.every((value) => value === 0)) {
      throw new InvalidStateError(
        `At least one value in feedforward must be non-zero`
      );
    }

    if (feedback.length === 0 || feedback.length > 20) {
      throw new NotSupportedError(
        `The length of feedback must be between 1 and 20, but got ${feedback.length}`
      );
    }

    if (feedback[0] === 0) {
      throw new InvalidStateError(
        `The first value of feedback must be non-zero`
      );
    }
  }
}
