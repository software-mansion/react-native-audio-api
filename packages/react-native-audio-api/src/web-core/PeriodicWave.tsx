import BaseAudioContext from './BaseAudioContext';
import { TPeriodicWaveOptions } from '../types';
import { generateRealAndImag } from '../core/PeriodicWave';

export default class PeriodicWave {
  /** @internal */
  readonly periodicWave: globalThis.PeriodicWave;

  constructor(context: BaseAudioContext, options?: TPeriodicWaveOptions) {
    const finalOptions = generateRealAndImag(options);
    const periodicWave = context.context.createPeriodicWave(
      finalOptions.real!,
      finalOptions.imag!,
      { disableNormalization: finalOptions.disableNormalization }
    );
    this.periodicWave = periodicWave;
  }
}
