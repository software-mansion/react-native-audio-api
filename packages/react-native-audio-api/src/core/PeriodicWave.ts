import { IPeriodicWave } from '../jsi-interfaces';
import type BaseAudioContext from './BaseAudioContext';
import { PeriodicWaveOptions } from '../types';
import { PeriodicWaveOptionsValidator } from '../utils/validation';

function toFloat32Array(
  values: number[] | Float32Array | undefined
): Float32Array | undefined {
  if (values === undefined) {
    return undefined;
  }
  return values instanceof Float32Array ? values : Float32Array.from(values);
}

export function generateRealAndImag(options?: PeriodicWaveOptions): {
  real: Float32Array;
  imag: Float32Array;
  disableNormalization: boolean;
} {
  let real: Float32Array;
  let imag: Float32Array;
  if (!options || (!options.real && !options.imag)) {
    real = new Float32Array(2);
    imag = new Float32Array(2);
    imag[1] = 1;
  } else {
    PeriodicWaveOptionsValidator.validate(options);
    const realIn = toFloat32Array(options.real);
    const imagIn = toFloat32Array(options.imag);
    if (realIn && imagIn) {
      real = realIn;
      imag = imagIn;
    } else if (realIn) {
      real = realIn;
      imag = new Float32Array(real.length);
    } else {
      imag = imagIn!;
      real = new Float32Array(imag.length);
    }
  }
  const disableNormalization = options?.disableNormalization
    ? options.disableNormalization
    : false;
  return { real, imag, disableNormalization };
}

export default class PeriodicWave {
  /** @internal */
  public readonly periodicWave: IPeriodicWave;

  constructor(context: BaseAudioContext, options?: PeriodicWaveOptions) {
    const finalOptions = generateRealAndImag(options);
    this.periodicWave = context.context.createPeriodicWave(
      finalOptions.real,
      finalOptions.imag,
      finalOptions.disableNormalization
    );
  }
}
