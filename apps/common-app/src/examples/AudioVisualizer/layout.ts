export interface VisualizerLayout {
  cx: number;
  cy: number;
  innerRadius: number;
  outerRadius: number;
  circleRadius: number;
}

export function getVisualizerLayout(width: number, height: number): VisualizerLayout {
  const base = Math.min(width, height);
  const innerRadius = base * 0.22;
  const outerRadius = base * 0.36;
  const circleRadius = base * 0.34;

  return {
    cx: width / 2,
    cy: height / 2,
    innerRadius,
    outerRadius,
    circleRadius,
  };
}

export function sampleExp(value: number) {
  return Math.exp(2.5 * value) / 4 - 0.2;
}

export function weightWithIndex(value: number, index: number, indexMax: number) {
  if (index < indexMax / 2) {
    return value * Math.max(index / (indexMax / 2), 0.5);
  }

  return value;
}
