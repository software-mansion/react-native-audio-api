import { IPeriodicWave } from '../interfaces';

export default class PeriodicWave {
  /** @internal */
  public readonly periodicWave: IPeriodicWave;

  constructor(periodicWave: IPeriodicWave) {
    this.periodicWave = periodicWave;
  }
}
