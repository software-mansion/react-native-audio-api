export function roundFromFloat32(value: number): number {
  return Number(value.toPrecision(7));
}
