import { IndexSizeError } from '../../errors';
import { OptionsValidator, PeriodicWaveOptions } from '../../types';

function assertFiniteSequence(
  values: ArrayLike<number>,
  label: 'real' | 'imag'
): void {
  for (let i = 0; i < values.length; i++) {
    if (!Number.isFinite(values[i])) {
      throw new TypeError(
        `Failed to construct 'PeriodicWave': The ${label} array must contain only finite values.`
      );
    }
  }
}

export const PeriodicWaveOptionsValidator: OptionsValidator<PeriodicWaveOptions> =
  {
    validate(options?: PeriodicWaveOptions): void {
      if (!options) {
        return;
      }

      const real = options.real;
      const imag = options.imag;

      if (real && imag) {
        if (real.length !== imag.length) {
          throw new IndexSizeError(
            `Failed to construct 'PeriodicWave': The lengths of the 'real' (${real.length}) and 'imag' (${imag.length}) arrays must match.`
          );
        }
        if (real.length < 2) {
          throw new IndexSizeError(
            `Failed to construct 'PeriodicWave': The length of the 'real' array (${real.length}) must be at least 2.`
          );
        }
        assertFiniteSequence(real, 'real');
        assertFiniteSequence(imag, 'imag');
        return;
      }

      if (real) {
        if (real.length < 2) {
          throw new IndexSizeError(
            `Failed to construct 'PeriodicWave': The length of the 'real' array (${real.length}) must be at least 2.`
          );
        }
        assertFiniteSequence(real, 'real');
        return;
      }

      if (imag) {
        if (imag.length < 2) {
          throw new IndexSizeError(
            `Failed to construct 'PeriodicWave': The length of the 'imag' array (${imag.length}) must be at least 2.`
          );
        }
        assertFiniteSequence(imag, 'imag');
      }
    },
  };
