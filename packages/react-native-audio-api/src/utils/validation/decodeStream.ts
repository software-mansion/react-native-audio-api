import { RangeError } from '../../errors';
import {
  MAX_SUPPORTED_SAMPLE_RATE,
  MIN_SUPPORTED_SAMPLE_RATE,
} from './sampleRate';

/** An omitted sample rate keeps the default: only a given one is checked. */
export function assertValidDecodeStreamSampleRate(
  sampleRate: number | undefined
): void {
  if (sampleRate === undefined) {
    return;
  }
  if (
    !Number.isFinite(sampleRate) ||
    sampleRate < MIN_SUPPORTED_SAMPLE_RATE ||
    sampleRate > MAX_SUPPORTED_SAMPLE_RATE
  ) {
    throw new RangeError(
      `sampleRate must be a number in [${MIN_SUPPORTED_SAMPLE_RATE}, ${MAX_SUPPORTED_SAMPLE_RATE}]: ${sampleRate}`
    );
  }
}

export function assertValidDecodeStreamFrameCount(frameCount: number): void {
  if (!Number.isSafeInteger(frameCount) || frameCount <= 0) {
    throw new RangeError(
      `frameCount must be a positive integer: ${frameCount}`
    );
  }
}

export function assertValidDecodeStreamPosition(seconds: number): void {
  if (!Number.isFinite(seconds) || seconds < 0) {
    throw new RangeError(
      `seconds must be a non-negative finite number: ${seconds}`
    );
  }
}
