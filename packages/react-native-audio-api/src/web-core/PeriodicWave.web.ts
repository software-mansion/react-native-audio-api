import BaseAudioContext from './BaseAudioContext.web';
import { PeriodicWaveOptions } from '../types';
import { normalizePeriodicWaveOptions } from '../utils/periodicWave';
import { PeriodicWaveOptionsValidator } from '../utils/validation';

export default class PeriodicWave {
  /** @internal */
  readonly periodicWave: globalThis.PeriodicWave;

  constructor(context: BaseAudioContext, options?: PeriodicWaveOptions) {
    PeriodicWaveOptionsValidator.validate(options);
    const finalOptions = normalizePeriodicWaveOptions(options);
    const periodicWave = context.context.createPeriodicWave(
      finalOptions.real,
      finalOptions.imag,
      { disableNormalization: finalOptions.disableNormalization }
    );
    this.periodicWave = periodicWave;
  }
}
