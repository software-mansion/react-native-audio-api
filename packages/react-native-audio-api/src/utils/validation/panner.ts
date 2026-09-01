import { InvalidStateError, NotSupportedError, RangeError } from '../../errors';
import type {
  AudioNodeOptions,
  ChannelCountMode,
  OptionsValidator,
  PannerOptions,
} from '../../types';
import { validateAudioNodeOptions } from './audioNodeOptions';

export const PannerOptionsValidator: OptionsValidator<PannerOptions> = {
  validate(options?: PannerOptions): void {
    if (!options) {
      return;
    }

    validateAudioNodeOptions(options);
    validatePannerChannelOptions(options);

    if (options.panningModel === 'HRTF') {
      throw new NotSupportedError(
        "panningModel 'HRTF' is not supported yet; use 'equalpower'"
      );
    }

    if (options.refDistance !== undefined && options.refDistance < 0) {
      throw new RangeError('refDistance cannot be set to a negative value');
    }

    if (options.maxDistance !== undefined && options.maxDistance <= 0) {
      throw new RangeError('maxDistance cannot be set to a non-positive value');
    }

    if (options.rolloffFactor !== undefined && options.rolloffFactor < 0) {
      throw new RangeError('rolloffFactor cannot be set to a negative value');
    }

    if (
      options.coneOuterGain !== undefined &&
      (options.coneOuterGain < 0 || options.coneOuterGain > 1)
    ) {
      throw new InvalidStateError('coneOuterGain must be in [0, 1]');
    }
  },
};

export function validatePannerChannelCount(channelCount: number): void {
  if (!Number.isInteger(channelCount) || channelCount < 1 || channelCount > 2) {
    throw new NotSupportedError(
      `The channelCount value (${channelCount}) must be 1 or 2 for a PannerNode`
    );
  }
}

export function validatePannerChannelCountMode(
  channelCountMode: ChannelCountMode
): void {
  if (channelCountMode === 'max') {
    throw new NotSupportedError(
      `The channelCountMode value ('max') is not supported for a PannerNode`
    );
  }
}

function validatePannerChannelOptions(options: AudioNodeOptions): void {
  if (options.channelCount !== undefined) {
    validatePannerChannelCount(options.channelCount);
  }

  if (options.channelCountMode !== undefined) {
    validatePannerChannelCountMode(options.channelCountMode);
  }
}
