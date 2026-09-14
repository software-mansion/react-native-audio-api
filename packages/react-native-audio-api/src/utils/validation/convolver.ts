import { NotSupportedError } from '../../errors';
import { ConvolverOptions, OptionsValidator } from '../../types';

export function validateConvolverBufferChannelCount(
  numberOfChannels: number
): void {
  if (
    numberOfChannels !== 1 &&
    numberOfChannels !== 2 &&
    numberOfChannels !== 4
  ) {
    throw new NotSupportedError(
      `The number of channels provided (${numberOfChannels}) in impulse response for ConvolverNode buffer must be 1 or 2 or 4.`
    );
  }
}

export function validateConvolverBufferSampleRate(
  bufferSampleRate: number,
  contextSampleRate: number
): void {
  if (bufferSampleRate !== contextSampleRate) {
    throw new NotSupportedError(
      `The sample rate of the impulse response for ConvolverNode buffer (${bufferSampleRate}) must match the sample rate of its context (${contextSampleRate}).`
    );
  }
}

export const ConvolverOptionsValidator: OptionsValidator<ConvolverOptions> = {
  validate(options?: ConvolverOptions): void {
    if (!options?.buffer) {
      return;
    }

    validateConvolverBufferChannelCount(options.buffer.numberOfChannels);
  },
};
