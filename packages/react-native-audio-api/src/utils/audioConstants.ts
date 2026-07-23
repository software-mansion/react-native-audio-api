import { NotSupportedError } from '../errors';

/** Web Audio API §2.4 Supported Sample Rates */
export const MIN_SUPPORTED_SAMPLE_RATE = 3000;
export const MAX_SUPPORTED_SAMPLE_RATE = 768000;

export function assertSupportedSampleRate(sampleRate: number): void {
  if (
    sampleRate < MIN_SUPPORTED_SAMPLE_RATE ||
    sampleRate > MAX_SUPPORTED_SAMPLE_RATE
  ) {
    throw new NotSupportedError(
      `The sample rate provided (${sampleRate}) is outside the range [${MIN_SUPPORTED_SAMPLE_RATE}, ${MAX_SUPPORTED_SAMPLE_RATE}]`
    );
  }
}
