
export default function lagrangeInterpolation(
  xTarget: number,
  xs: number[],
  ys: number[]
): number {
  const n = xs.length;
  let result = 0;

  for (let i = 0; i < n; i++) {
    let term = ys[i];
    for (let j = 0; j < n; j++) {
      if (j !== i) {
        term *= (xTarget - xs[j]) / (xs[i] - xs[j]);
      }
    }
    result += term;
  }

  return result;
}
