import { NotSupportedError } from '../../errors';
import { AudioNodeOptions } from '../../types';

export const MAX_CHANNEL_COUNT = 32;
const VALID_CHANNEL_COUNT_MODES = ['max', 'clamped-max', 'explicit'];
const VALID_CHANNEL_INTERPRETATIONS = ['speakers', 'discrete'];

/** Validates a `channelCount` attribute value (constructor options or setter). */
export function validateChannelCount(channelCount: number): void {
  if (
    !Number.isInteger(channelCount) ||
    channelCount < 1 ||
    channelCount > MAX_CHANNEL_COUNT
  ) {
    throw new NotSupportedError(
      `The channelCount value (${channelCount}) must be an integer between 1 and ${MAX_CHANNEL_COUNT}`
    );
  }
}

export function validateAudioNodeOptions(options?: AudioNodeOptions): void {
  if (!options) {
    return;
  }

  if (options.channelCount !== undefined) {
    validateChannelCount(options.channelCount);
  }

  if (
    options.channelCountMode !== undefined &&
    !VALID_CHANNEL_COUNT_MODES.includes(options.channelCountMode)
  ) {
    throw new TypeError(
      `The channelCountMode value ('${options.channelCountMode}') is not a valid ChannelCountMode type`
    );
  }

  if (
    options.channelInterpretation !== undefined &&
    !VALID_CHANNEL_INTERPRETATIONS.includes(options.channelInterpretation)
  ) {
    throw new TypeError(
      `The channelInterpretation value ('${options.channelInterpretation}') is not a valid ChannelInterpretation type`
    );
  }
}
