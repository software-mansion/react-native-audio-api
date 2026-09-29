/**
 * Web IDL `float` is a single-precision value; reject non-representable
 * numbers.
 */
export function assertFloat32Representable(value: number): void {
  if (typeof value !== 'number') {
    throw new TypeError();
  }

  if (!Number.isFinite(Math.fround(value))) {
    throw new TypeError();
  }
}
