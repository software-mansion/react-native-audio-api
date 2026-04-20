function clamp(value: number, min: number, max: number): number {
  return Math.min(max, Math.max(min, value));
}

function getPrecision(value: number): number {
  if (!Number.isFinite(value)) {
    return 0;
  }

  const normalized = value.toString().toLowerCase();
  const [base, exponent] = normalized.split('e-');

  if (exponent != null) {
    const decimalPartLength = (base.split('.')[1] ?? '').length;
    return Number(exponent) + decimalPartLength;
  }

  return (normalized.split('.')[1] ?? '').length;
}

function snapToStep(value: number, min: number, step: number): number {
  if (!Number.isFinite(step) || step <= 0) {
    return value;
  }

  const steps = Math.round((value - min) / step);
  return min + steps * step;
}

export function normalizeValue(value: number, min: number, max: number, step: number): number {
  const clamped = clamp(value, min, max);
  const snapped = snapToStep(clamped, min, step);
  const bounded = clamp(snapped, min, max);
  const precision = Math.max(getPrecision(step), getPrecision(min), getPrecision(max));

  return Number(bounded.toFixed(precision));
}

export function formatValue(value: number, step: number): string {
  const precision = Math.max(1, getPrecision(step));
  return value.toFixed(precision);
}
