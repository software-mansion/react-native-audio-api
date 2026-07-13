import { NotSupportedError } from '../../errors';
import { OptionsValidator, PeriodicWaveOptions } from '../../types';

export const PeriodicWaveOptionsValidator: OptionsValidator<PeriodicWaveOptions> =
  {
    validate(options?: PeriodicWaveOptions): void {
      if (!options) {
        return;
      }

      if (
        options.real &&
        options.imag &&
        options.real.length !== options.imag.length
      ) {
        throw new NotSupportedError(
          "'real' and 'imag' arrays must have the same length"
        );
      }
    },
  };
