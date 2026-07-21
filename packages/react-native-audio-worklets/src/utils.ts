import { IndexSizeError } from 'react-native-audio-api';

import type { WorkletNodeDomain, WorkletNodeOptions } from './types';

export const WORKLET_ALLOWED_BUFFER_LENGTH = [
  32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
] as const;

const DEFAULT_BUFFER_LENGTH = 1024;
const DEFAULT_SMOOTHING_TIME_CONSTANT = 0.8;

export function validateWorkletBufferLength(bufferLength: number): void {
  if (
    !WORKLET_ALLOWED_BUFFER_LENGTH.includes(
      bufferLength as (typeof WORKLET_ALLOWED_BUFFER_LENGTH)[number]
    )
  ) {
    throw new IndexSizeError(
      `Provided value (${bufferLength}) must be a power of 2 between 32 and 32768`
    );
  }
}

export function validateWorkletSmoothingTimeConstant(
  smoothingTimeConstant: number
): void {
  if (smoothingTimeConstant < 0 || smoothingTimeConstant > 1) {
    throw new IndexSizeError(
      `The smoothingTimeConstant value (${smoothingTimeConstant}) must be between 0 and 1`
    );
  }
}

export const WorkletNodeOptionsValidator = {
  validate(options?: WorkletNodeOptions): void {
    if (!options) {
      return;
    }

    if (options.bufferLength !== undefined) {
      validateWorkletBufferLength(options.bufferLength);
    }

    if (options.smoothingTimeConstant !== undefined) {
      validateWorkletSmoothingTimeConstant(options.smoothingTimeConstant);
    }
  },
};

export function resolveWorkletNodeOptions(
  options?: number | WorkletNodeOptions
): {
  domain: WorkletNodeDomain;
  bufferLength: number;
  smoothingTimeConstant: number;
} {
  if (options === undefined) {
    const bufferLength = DEFAULT_BUFFER_LENGTH;
    validateWorkletBufferLength(bufferLength);
    return {
      domain: 'time-domain',
      bufferLength,
      smoothingTimeConstant: DEFAULT_SMOOTHING_TIME_CONSTANT,
    };
  }

  if (typeof options === 'number') {
    validateWorkletBufferLength(options);
    return {
      domain: 'time-domain',
      bufferLength: options,
      smoothingTimeConstant: DEFAULT_SMOOTHING_TIME_CONSTANT,
    };
  }

  WorkletNodeOptionsValidator.validate(options);

  const domain = options.domain ?? 'time-domain';
  const bufferLength = options.bufferLength ?? DEFAULT_BUFFER_LENGTH;

  validateWorkletBufferLength(bufferLength);

  return {
    domain,
    bufferLength,
    smoothingTimeConstant:
      options.smoothingTimeConstant ?? DEFAULT_SMOOTHING_TIME_CONSTANT,
  };
}
