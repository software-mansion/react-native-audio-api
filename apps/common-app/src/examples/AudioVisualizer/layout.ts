/** Proportions of the radial visualizer, as fractions of the square canvas. */
export interface VisualizerLayout {
  cx: number;
  cy: number;
  /** Where the frequency bars start. */
  barRadius: number;
  /** Length of a bar at full magnitude. */
  barMaxLength: number;
  barWidth: number;
  /** Rest radius of the time-domain ring. */
  ringRadius: number;
  /** How far the ring swings in or out at full-scale signal. */
  ringAmplitude: number;
  ringWidth: number;
}

export function getVisualizerLayout(side: number): VisualizerLayout {
  return {
    cx: side / 2,
    cy: side / 2,
    barRadius: side * 0.28,
    barMaxLength: side * 0.18,
    barWidth: side * 0.0128,
    ringRadius: side * 0.21,
    ringAmplitude: side * 0.06,
    ringWidth: side * 0.0064,
  };
}
