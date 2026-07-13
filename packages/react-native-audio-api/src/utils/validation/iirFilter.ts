import { InvalidStateError, NotSupportedError } from '../../errors';
import { IIRFilterOptions } from '../../types';

export function validateIIRFilterOptions(options: IIRFilterOptions): void {
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
    throw new InvalidStateError(`The first value of feedback must be non-zero`);
  }
}
