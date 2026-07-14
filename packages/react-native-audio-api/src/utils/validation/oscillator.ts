import { NotSupportedError } from '../../errors';
import { OscillatorOptions, OptionsValidator } from '../../types';

export const OscillatorOptionsValidator: OptionsValidator<OscillatorOptions> = {
  validate(options?: OscillatorOptions): void {
    if (!options) {
      return;
    }

    if (options.type === 'custom' && !options.periodicWave) {
      throw new NotSupportedError(
        "'type' cannot be set to 'custom' without providing a 'periodicWave'."
      );
    }
  },
};
