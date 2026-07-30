import { IndexSizeError } from '../../errors';
import { OptionsValidator, PeriodicWaveOptions } from '../../types';
import { assertFiniteSequence } from '..';

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
        assertFiniteSequence(
          real,
          `Failed to construct 'PeriodicWave': The real array must contain only finite values.`
        );
        assertFiniteSequence(
          imag,
          `Failed to construct 'PeriodicWave': The imag array must contain only finite values.`
        );
        return;
      }

      if (real) {
        if (real.length < 2) {
          throw new IndexSizeError(
            `Failed to construct 'PeriodicWave': The length of the 'real' array (${real.length}) must be at least 2.`
          );
        }
        assertFiniteSequence(
          real,
          `Failed to construct 'PeriodicWave': The real array must contain only finite values.`
        );
        return;
      }

      if (imag) {
        if (imag.length < 2) {
          throw new IndexSizeError(
            `Failed to construct 'PeriodicWave': The length of the 'imag' array (${imag.length}) must be at least 2.`
          );
        }
        assertFiniteSequence(
          imag,
          `Failed to construct 'PeriodicWave': The imag array must contain only finite values.`
        );
      }
    },
  };
