import { IndexSizeError, NotSupportedError } from './errors';
import {
  OptionsValidator,
  TAnalyserOptions,
  TConvolverOptions,
  TOscillatorOptions,
  TPeriodicWaveOptions,
} from './types';

export const AnalyserOptionsValidator: OptionsValidator<TAnalyserOptions> = {
  validate(options?: TAnalyserOptions): void {
    if (!options) {
      return;
    }
    const allowedFFTSize: number[] = [
      32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
    ];
    if (options.fftSize && !allowedFFTSize.includes(options.fftSize)) {
      throw new IndexSizeError(
        `fftSize must be one of the following values: ${allowedFFTSize.join(
          ', '
        )}`
      );
    }
  },
};

export const ConvolverOptionsValidator: OptionsValidator<TConvolverOptions> = {
  validate(options?: TConvolverOptions): void {
    if (!options) {
      return;
    }
    if (options.buffer) {
      const numberOfChannels = options.buffer.numberOfChannels;
      if (
        numberOfChannels !== 1 &&
        numberOfChannels !== 2 &&
        numberOfChannels !== 4
      ) {
        throw new NotSupportedError(
          `The number of channels provided (${numberOfChannels}) in impulse response for ConvolverNode buffer must be 1 or 2 or 4.`
        );
      }
    }
  },
};

export const OscillatorOptionsValidator: OptionsValidator<TOscillatorOptions> =
  {
    validate(options?: TOscillatorOptions): void {
      if (!options) {
        return;
      }
      if (options.type === 'custom' && !options.periodicWave) {
        throw new NotSupportedError(
          "'type' cannot be set to 'custom' without providing a 'periodicWave'."
        );
      }
    },
  };

export const PeriodicWaveOptionsValidator: OptionsValidator<TPeriodicWaveOptions> =
  {
    validate(options?: TPeriodicWaveOptions): void {
      if (!options) {
        return;
      }
      if (
        options.real &&
        options.imag &&
        options.real.length !== options.imag.length
      ) {
        throw new NotSupportedError(
          "'real' and 'imag' arrays must have the same length"
        );
      }
    },
  };
