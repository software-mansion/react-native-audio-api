import { NotSupportedError } from '../../errors';
import {
  ChannelCountMode,
  ConvolverOptions,
  OptionsValidator,
} from '../../types';

const MAX_CONVOLVER_CHANNEL_COUNT = 2;

/**
 * Spec channel limitation: a ConvolverNode processes at most stereo input, so
 * `channelCount` above 2 is a NotSupportedError (constructor and setter).
 */
export function validateConvolverChannelCount(channelCount: number): void {
  if (channelCount > MAX_CONVOLVER_CHANNEL_COUNT) {
    throw new NotSupportedError(
      `The channelCount value (${channelCount}) of ConvolverNode must be 1 or 2.`
    );
  }
}

/**
 * Spec channel limitation: `max` would let a multichannel input bypass the
 * stereo limit, so only `clamped-max` and `explicit` are allowed.
 */
export function validateConvolverChannelCountMode(
  channelCountMode: ChannelCountMode
): void {
  if (channelCountMode === 'max') {
    throw new NotSupportedError(
      `The channelCountMode value ('max') is not supported by ConvolverNode; use 'clamped-max' or 'explicit'.`
    );
  }
}

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
    if (!options) {
      return;
    }

    if (options.channelCount !== undefined) {
      validateConvolverChannelCount(options.channelCount);
    }

    if (options.channelCountMode !== undefined) {
      validateConvolverChannelCountMode(options.channelCountMode);
    }

    if (options.buffer) {
      validateConvolverBufferChannelCount(options.buffer.numberOfChannels);
    }
  },
};
