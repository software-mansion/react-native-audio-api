import { IPeriodicWave } from '../interfaces';
import BaseAudioContext from './BaseAudioContext';
import { TPeriodicWaveOptions } from '../types';
import { PeriodicWaveConstraints } from '../defaults';
import { PeriodicWaveOptionsValidator } from '../options-validators';

export function generateRealAndImag(
  options?: TPeriodicWaveOptions
): TPeriodicWaveOptions {
  let real: Float32Array | undefined;
  let imag: Float32Array | undefined;
  if (!options || (!options.real && !options.imag)) {
    real = new Float32Array(2);
    imag = new Float32Array(2);
    imag[1] = 1;
  } else {
    real = options.real;
    imag = options.imag;
    PeriodicWaveOptionsValidator.validate(options);
    if (real && !imag) {
      imag = new Float32Array(real.length);
    } else if (!real && imag) {
      real = new Float32Array(imag.length);
    }
  }
  const norm: boolean = options?.disableNormalization
    ? options.disableNormalization
    : PeriodicWaveConstraints.disableNormalization!;
  return { real, imag, disableNormalization: norm };
}

export default class PeriodicWave {
  /** @internal */
  public readonly periodicWave: IPeriodicWave;

  constructor(context: BaseAudioContext, options?: TPeriodicWaveOptions) {
    const finalOptions = generateRealAndImag(options);
    this.periodicWave = context.context.createPeriodicWave(
      finalOptions.real!,
      finalOptions.imag!,
      finalOptions.disableNormalization!
    );
  }
}
