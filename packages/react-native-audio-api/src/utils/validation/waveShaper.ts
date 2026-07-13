import { InvalidStateError } from '../../errors';

export function validateWaveShaperCurve(
  curve: Float32Array | null,
  isCurveSet: boolean
): void {
  if (curve === null) {
    return;
  }

  if (isCurveSet) {
    throw new InvalidStateError(
      'The curve can only be set once and cannot be changed afterwards.'
    );
  }

  if (curve.length < 2) {
    throw new InvalidStateError(
      'The curve must have at least two values if not null.'
    );
  }
}
