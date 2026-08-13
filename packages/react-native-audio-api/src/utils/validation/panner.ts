import { InvalidStateError, NotSupportedError, RangeError } from '../../errors';
import type { OptionsValidator, PannerOptions } from '../../types';

export const PannerOptionsValidator: OptionsValidator<PannerOptions> = {
  validate(options?: PannerOptions): void {
    if (!options) {
      return;
    }

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
