import { IndexSizeError, InvalidStateError } from '../../errors';
import { ChannelMergerOptions, ChannelSplitterOptions } from '../../types';
import { validateAudioNodeOptions } from './audioNodeOptions';

const MAX_CHANNEL_COUNT = 32;
const DEFAULT_SPLITTER_OUTPUTS = 6;

export function validateChannelMergerOptions(
  options?: ChannelMergerOptions
): void {
  if (!options) {
    return;
  }

  // Reject invalid enum strings (TypeError) before fixed-attribute checks.
  validateAudioNodeOptions(options);

  if (options.numberOfInputs !== undefined) {
    const { numberOfInputs } = options;
    if (
      !Number.isInteger(numberOfInputs) ||
      numberOfInputs < 1 ||
      numberOfInputs > MAX_CHANNEL_COUNT
    ) {
      throw new IndexSizeError(
        `The numberOfInputs value (${numberOfInputs}) is outside the range [1, ${MAX_CHANNEL_COUNT}]`
      );
    }
  }

  if (options.channelCount !== undefined && options.channelCount !== 1) {
    throw new InvalidStateError(
      `ChannelMergerNode channelCount cannot be changed from 1`
    );
  }

  if (
    options.channelCountMode !== undefined &&
    options.channelCountMode !== 'explicit'
  ) {
    throw new InvalidStateError(
      `ChannelMergerNode channelCountMode cannot be changed from 'explicit'`
    );
  }
}

export function validateChannelSplitterOptions(
  options?: ChannelSplitterOptions
): void {
  if (!options) {
    return;
  }

  validateAudioNodeOptions(options);

  const numberOfOutputs = options.numberOfOutputs ?? DEFAULT_SPLITTER_OUTPUTS;

  if (options.numberOfOutputs !== undefined) {
    if (
      !Number.isInteger(numberOfOutputs) ||
      numberOfOutputs < 1 ||
      numberOfOutputs > MAX_CHANNEL_COUNT
    ) {
      throw new IndexSizeError(
        `The numberOfOutputs value (${numberOfOutputs}) is outside the range [1, ${MAX_CHANNEL_COUNT}]`
      );
    }
  }

  if (
    options.channelCount !== undefined &&
    options.channelCount !== numberOfOutputs
  ) {
    throw new InvalidStateError(
      `ChannelSplitterNode channelCount cannot be changed from ${numberOfOutputs}`
    );
  }

  if (
    options.channelCountMode !== undefined &&
    options.channelCountMode !== 'explicit'
  ) {
    throw new InvalidStateError(
      `ChannelSplitterNode channelCountMode cannot be changed from 'explicit'`
    );
  }

  if (
    options.channelInterpretation !== undefined &&
    options.channelInterpretation !== 'discrete'
  ) {
    throw new InvalidStateError(
      `ChannelSplitterNode channelInterpretation cannot be changed from 'discrete'`
    );
  }
}
