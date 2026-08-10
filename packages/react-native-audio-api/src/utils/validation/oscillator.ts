import { InvalidStateError } from '../../errors';
import { OscillatorOptions, OptionsValidator } from '../../types';

export const OscillatorOptionsValidator: OptionsValidator<OscillatorOptions> = {
  validate(options?: OscillatorOptions): void {
    if (!options) {
      return;
    }

    // WebIDL: PeriodicWave is a non-nullable interface type. Passing null throws TypeError.
    if (
      Object.prototype.hasOwnProperty.call(options, 'periodicWave') &&
      options.periodicWave == null
    ) {
      throw new TypeError(
        "Failed to construct 'OscillatorNode': Failed to read the 'periodicWave' property from 'OscillatorOptions': Failed to convert value to 'PeriodicWave'."
      );
    }

    // Spec: type "custom" without a PeriodicWave throws InvalidStateError.
    if (options.type === 'custom' && !options.periodicWave) {
      throw new InvalidStateError(
        "'type' cannot be set to 'custom' without providing a 'periodicWave'."
      );
    }
  },
};
