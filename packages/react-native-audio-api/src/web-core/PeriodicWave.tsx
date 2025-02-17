export default class PeriodicWave {
  /** @internal */
  readonly periodicWave: globalThis.PeriodicWave;

  constructor(periodicWave: globalThis.PeriodicWave) {
    this.periodicWave = periodicWave;
  }
}
