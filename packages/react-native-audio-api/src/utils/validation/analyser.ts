import { IndexSizeError } from '../../errors';
import { AnalyserOptions, OptionsValidator } from '../../types';

export const ANALYSER_ALLOWED_FFT_SIZE = [
  32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
];

const ANALYSER_DEFAULT_MIN_DECIBELS = -100;
const ANALYSER_DEFAULT_MAX_DECIBELS = -30;

export function validateAnalyserFftSize(fftSize: number): void {
  if (!ANALYSER_ALLOWED_FFT_SIZE.includes(fftSize)) {
    throw new IndexSizeError(
      `Provided value (${fftSize}) must be a power of 2 between 32 and 32768`
    );
  }
}

export function validateAnalyserMinDecibels(
  minDecibels: number,
  maxDecibels: number
): void {
  if (minDecibels >= maxDecibels) {
    throw new IndexSizeError(
      `The minDecibels value (${minDecibels}) must be less than maxDecibels`
    );
  }
}

export function validateAnalyserMaxDecibels(
  maxDecibels: number,
  minDecibels: number
): void {
  if (maxDecibels <= minDecibels) {
    throw new IndexSizeError(
      `The maxDecibels value (${maxDecibels}) must be greater than minDecibels`
    );
  }
}

export function validateAnalyserSmoothingTimeConstant(
  smoothingTimeConstant: number
): void {
  if (smoothingTimeConstant < 0 || smoothingTimeConstant > 1) {
    throw new IndexSizeError(
      `The smoothingTimeConstant value (${smoothingTimeConstant}) must be between 0 and 1`
    );
  }
}

export const AnalyserOptionsValidator: OptionsValidator<AnalyserOptions> = {
  validate(options?: AnalyserOptions): void {
    if (!options) {
      return;
    }

    if (options.fftSize !== undefined) {
      validateAnalyserFftSize(options.fftSize);
    }

    const minDecibels = options.minDecibels ?? ANALYSER_DEFAULT_MIN_DECIBELS;
    const maxDecibels = options.maxDecibels ?? ANALYSER_DEFAULT_MAX_DECIBELS;

    if (options.minDecibels !== undefined) {
      validateAnalyserMinDecibels(options.minDecibels, maxDecibels);
    }

    if (options.maxDecibels !== undefined) {
      validateAnalyserMaxDecibels(options.maxDecibels, minDecibels);
    }

    if (options.smoothingTimeConstant !== undefined) {
      validateAnalyserSmoothingTimeConstant(options.smoothingTimeConstant);
    }
  },
};
