import { IndexSizeError, NotSupportedError } from './errors';
import {
  OptionsValidator,
  AnalyserOptions,
  AudioNodeOptions,
  BiquadFilterOptions,
  ConvolverOptions,
  OscillatorOptions,
  PeriodicWaveOptions,
} from './types';

const MAX_CHANNEL_COUNT = 32;
const VALID_CHANNEL_COUNT_MODES = ['max', 'clamped-max', 'explicit'];
const VALID_CHANNEL_INTERPRETATIONS = ['speakers', 'discrete'];

export function validateAudioNodeOptions(options?: AudioNodeOptions): void {
  if (!options) {
    return;
  }

  if (options.channelCount !== undefined) {
    const { channelCount } = options;
    if (
      !Number.isInteger(channelCount) ||
      channelCount < 1 ||
      channelCount > MAX_CHANNEL_COUNT
    ) {
      throw new NotSupportedError(
        `The channelCount value (${channelCount}) must be an integer between 1 and ${MAX_CHANNEL_COUNT}`
      );
    }
  }

  if (
    options.channelCountMode !== undefined &&
    !VALID_CHANNEL_COUNT_MODES.includes(options.channelCountMode)
  ) {
    throw new TypeError(
      `The channelCountMode value ('${options.channelCountMode}') is not a valid enum value of type ChannelCountMode`
    );
  }

  if (
    options.channelInterpretation !== undefined &&
    !VALID_CHANNEL_INTERPRETATIONS.includes(options.channelInterpretation)
  ) {
    throw new TypeError(
      `The channelInterpretation value ('${options.channelInterpretation}') is not a valid enum value of type ChannelInterpretation`
    );
  }
}

const ANALYSER_DEFAULT_MIN_DECIBELS = -100;
const ANALYSER_DEFAULT_MAX_DECIBELS = -30;

export const ANALYSER_ALLOWED_FFT_SIZE = [
  32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
];

function validateAnalyserMinDecibels(
  minDecibels: number,
  maxDecibels: number
): void {
  if (minDecibels >= maxDecibels) {
    throw new IndexSizeError(
      `The minDecibels value (${minDecibels}) must be less than maxDecibels`
    );
  }
}

function validateAnalyserMaxDecibels(
  maxDecibels: number,
  minDecibels: number
): void {
  if (maxDecibels <= minDecibels) {
    throw new IndexSizeError(
      `The maxDecibels value (${maxDecibels}) must be greater than minDecibels`
    );
  }
}

export const AnalyserOptionsValidator: OptionsValidator<AnalyserOptions> = {
  validate(options?: AnalyserOptions): void {
    if (!options) {
      return;
    }

    validateAudioNodeOptions(options);

    if (
      options.fftSize !== undefined &&
      !ANALYSER_ALLOWED_FFT_SIZE.includes(options.fftSize)
    ) {
      throw new IndexSizeError(
        `Provided value (${options.fftSize}) must be a power of 2 between 32 and 32768`
      );
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
      const smoothingTimeConstant = options.smoothingTimeConstant;
      if (smoothingTimeConstant < 0 || smoothingTimeConstant > 1) {
        throw new IndexSizeError(
          `The smoothingTimeConstant value (${smoothingTimeConstant}) must be between 0 and 1`
        );
      }
    }
  },
};

export const BiquadFilterOptionsValidator: OptionsValidator<BiquadFilterOptions> =
  {
    validate(options?: BiquadFilterOptions): void {
      validateAudioNodeOptions(options);
    },
  };

export const ConvolverOptionsValidator: OptionsValidator<ConvolverOptions> = {
  validate(options?: ConvolverOptions): void {
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

export const OscillatorOptionsValidator: OptionsValidator<OscillatorOptions> = {
  validate(options?: OscillatorOptions): void {
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

export const PeriodicWaveOptionsValidator: OptionsValidator<PeriodicWaveOptions> =
  {
    validate(options?: PeriodicWaveOptions): void {
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
