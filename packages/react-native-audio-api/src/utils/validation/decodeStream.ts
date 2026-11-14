import { RangeError } from '../../errors';

export function assertValidDecodeStreamFrameCount(frameCount: number): void {
  if (!Number.isInteger(frameCount) || frameCount <= 0) {
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
