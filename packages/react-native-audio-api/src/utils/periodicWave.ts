import type { PeriodicWaveOptions } from '../types';
import { toFloat32Array } from '.';

export type NormalizedPeriodicWaveOptions = {
  real: Float32Array;
  imag: Float32Array;
  disableNormalization: boolean;
};

export function normalizePeriodicWaveOptions(
  options?: PeriodicWaveOptions
): NormalizedPeriodicWaveOptions {
  let real: Float32Array;
  let imag: Float32Array;

  if (!options || (!options.real && !options.imag)) {
    real = new Float32Array(2);
    imag = new Float32Array(2);
    imag[1] = 1;
  } else {
    const realInput = toFloat32Array(options.real);
    const imagInput = toFloat32Array(options.imag);

    if (realInput && imagInput) {
      real = realInput;
      imag = imagInput;
    } else if (realInput) {
      real = realInput;
      imag = new Float32Array(real.length);
    } else {
      imag = imagInput!;
      real = new Float32Array(imag.length);
    }
  }

  return {
    real,
    imag,
    disableNormalization: options?.disableNormalization ?? false,
  };
}
