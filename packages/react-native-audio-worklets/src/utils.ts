import { NotSupportedError } from 'react-native-audio-api';

export function validateBufferLength(bufferLength: unknown): number {
  if (typeof bufferLength !== 'number') {
    throw new NotSupportedError(
      `bufferLength must be a number, got ${typeof bufferLength}`
    );
  }

  if (!Number.isFinite(bufferLength)) {
    throw new NotSupportedError(
      `bufferLength must be a finite number, got ${bufferLength}`
    );
  }

  if (!Number.isInteger(bufferLength)) {
    throw new NotSupportedError(
      `bufferLength must be an integer, got ${bufferLength}`
    );
  }

  if (bufferLength < 1) {
    throw new NotSupportedError(
      `bufferLength must be greater than or equal to 1, got ${bufferLength}`
    );
  }

  return bufferLength;
}
