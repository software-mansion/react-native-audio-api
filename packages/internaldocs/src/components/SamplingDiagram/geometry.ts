export const VIEW_WIDTH = 720;
export const VIEW_HEIGHT = 330;

export const PLOT_LEFT = 70;
export const PLOT_RIGHT = 680;
export const AXIS_Y = 165;
export const AXIS_TOP = 48;
export const AXIS_BOTTOM = 290;

const AMPLITUDE = 100;
const PERIODS = 2;
const SAMPLES_PER_PERIOD = 12;
const SAMPLE_COUNT = PERIODS * SAMPLES_PER_PERIOD;
const SAMPLE_SPACING = (PLOT_RIGHT - 30 - PLOT_LEFT) / SAMPLE_COUNT;
const CURVE_SEGMENTS = 240;

/** Index of the first of the two neighbouring samples the period bracket spans. */
const BRACKET_SAMPLE_INDEX = 14;

export interface Point {
  x: number;
  y: number;
}

function signalAt(phase: number): number {
  return AXIS_Y - AMPLITUDE * Math.sin(2 * Math.PI * phase);
}

function round(value: number): number {
  return Math.round(value * 10) / 10;
}

export const samples: Point[] = Array.from(
  { length: SAMPLE_COUNT + 1 },
  (_, index) => ({
    x: round(PLOT_LEFT + index * SAMPLE_SPACING),
    y: round(signalAt(index / SAMPLES_PER_PERIOD)),
  })
);

export const continuousSignalPath = Array.from(
  { length: CURVE_SEGMENTS + 1 },
  (_, index) => {
    const progress = index / CURVE_SEGMENTS;
    const x = round(PLOT_LEFT + progress * SAMPLE_COUNT * SAMPLE_SPACING);
    const y = round(signalAt(progress * PERIODS));
    return `${index === 0 ? 'M' : 'L'}${x},${y}`;
  }
).join(' ');

/** Each sample's value held flat until the next sample arrives. */
export const heldSignalPath = samples
  .map((sample, index) =>
    index === 0 ? `M${sample.x},${sample.y}` : `H${sample.x} V${sample.y}`
  )
  .join(' ');

export const periodBracket = {
  from: samples[BRACKET_SAMPLE_INDEX],
  to: samples[BRACKET_SAMPLE_INDEX + 1],
  y: 34,
};
