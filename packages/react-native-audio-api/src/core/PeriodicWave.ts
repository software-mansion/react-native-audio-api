import { IPeriodicWave } from '../jsi-interfaces';
import type BaseAudioContext from './BaseAudioContext';
import { PeriodicWaveOptions } from '../types';
import { normalizePeriodicWaveOptions } from '../utils/periodicWave';
import { PeriodicWaveOptionsValidator } from '../utils/validation';

export default class PeriodicWave {
  /** @internal */
  public readonly periodicWave: IPeriodicWave;

  constructor(context: BaseAudioContext, options?: PeriodicWaveOptions) {
    PeriodicWaveOptionsValidator.validate(options);
    const finalOptions = normalizePeriodicWaveOptions(options);
    this.periodicWave = context.context.createPeriodicWave(
      finalOptions.real,
      finalOptions.imag,
      finalOptions.disableNormalization
    );
  }
}
