import React from 'react';

import {
  AXIS_BOTTOM,
  AXIS_TOP,
  AXIS_Y,
  PLOT_LEFT,
  PLOT_RIGHT,
  VIEW_HEIGHT,
  VIEW_WIDTH,
  continuousSignalPath,
  heldSignalPath,
  periodBracket,
  samples,
} from './geometry';
import styles from './styles.module.css';

const ARROW_LENGTH = 9;
const ARROW_HALF_WIDTH = 4.5;
const BRACKET_TICK = 5;
const LEGEND_Y = 316;

function arrowHead(x: number, y: number, direction: 'up' | 'down' | 'right') {
  const points = {
    up: `${x},${y} ${x - ARROW_HALF_WIDTH},${y + ARROW_LENGTH} ${x + ARROW_HALF_WIDTH},${y + ARROW_LENGTH}`,
    down: `${x},${y} ${x - ARROW_HALF_WIDTH},${y - ARROW_LENGTH} ${x + ARROW_HALF_WIDTH},${y - ARROW_LENGTH}`,
    right: `${x},${y} ${x - ARROW_LENGTH},${y - ARROW_HALF_WIDTH} ${x - ARROW_LENGTH},${y + ARROW_HALF_WIDTH}`,
  };

  return <polygon className={styles.axisArrow} points={points[direction]} />;
}

export default function SamplingDiagram() {
  const bracketCenterX = (periodBracket.from.x + periodBracket.to.x) / 2;

  return (
    <figure className={styles.figure}>
      <svg
        viewBox={`0 0 ${VIEW_WIDTH} ${VIEW_HEIGHT}`}
        role="img"
        aria-labelledby="sampling-diagram-title sampling-diagram-desc"
      >
        <title id="sampling-diagram-title">Sampling a continuous signal</title>
        <desc id="sampling-diagram-desc">
          A sine wave measured at evenly spaced moments. Each measurement is a
          sample, and the time between two neighbouring samples is one divided
          by the sample rate.
        </desc>

        {samples.map((sample, index) => (
          <line
            key={index}
            className={styles.stem}
            x1={sample.x}
            x2={sample.x}
            y1={AXIS_Y}
            y2={sample.y}
          />
        ))}

        <line
          className={styles.axis}
          x1={PLOT_LEFT}
          x2={PLOT_LEFT}
          y1={AXIS_TOP}
          y2={AXIS_BOTTOM}
        />
        {arrowHead(PLOT_LEFT, AXIS_TOP - ARROW_LENGTH + 1, 'up')}
        {arrowHead(PLOT_LEFT, AXIS_BOTTOM + ARROW_LENGTH - 1, 'down')}
        <line
          className={styles.axis}
          x1={PLOT_LEFT}
          x2={PLOT_RIGHT}
          y1={AXIS_Y}
          y2={AXIS_Y}
        />
        {arrowHead(PLOT_RIGHT + ARROW_LENGTH - 1, AXIS_Y, 'right')}

        <text
          className={styles.label}
          textAnchor="middle"
          transform={`translate(${PLOT_LEFT - 28} ${AXIS_Y}) rotate(-90)`}
        >
          Amplitude
        </text>
        <text
          className={styles.label}
          textAnchor="end"
          x={PLOT_RIGHT + ARROW_LENGTH}
          y={AXIS_Y + 24}
        >
          Time
        </text>

        <path className={styles.heldSignal} d={heldSignalPath} />
        <path className={styles.continuousSignal} d={continuousSignalPath} />

        {samples.map((sample, index) => (
          <circle
            key={index}
            className={styles.sample}
            cx={sample.x}
            cy={sample.y}
            r={4}
          />
        ))}

        <g className={styles.bracket}>
          <line
            x1={periodBracket.from.x}
            x2={periodBracket.from.x}
            y1={periodBracket.y - BRACKET_TICK}
            y2={periodBracket.from.y - 8}
          />
          <line
            x1={periodBracket.to.x}
            x2={periodBracket.to.x}
            y1={periodBracket.y - BRACKET_TICK}
            y2={periodBracket.to.y - 8}
          />
          <line
            x1={periodBracket.from.x}
            x2={periodBracket.to.x}
            y1={periodBracket.y}
            y2={periodBracket.y}
          />
        </g>
        <text
          className={styles.label}
          textAnchor="middle"
          x={bracketCenterX}
          y={periodBracket.y - 14}
        >
          sampling period = 1 / sample rate
        </text>

        <g transform={`translate(${PLOT_LEFT + 20} ${LEGEND_Y})`}>
          <line
            className={styles.continuousSignal}
            x1={0}
            x2={28}
            y1={0}
            y2={0}
          />
          <text className={styles.legend} x={36} y={4}>
            continuous signal
          </text>

          <circle className={styles.sample} cx={196} cy={0} r={4} />
          <text className={styles.legend} x={208} y={4}>
            sample
          </text>

          <path className={styles.heldSignal} d="M290,5 H304 V-5 H318" />
          <text className={styles.legend} x={326} y={4}>
            value held until the next sample
          </text>
        </g>
      </svg>
    </figure>
  );
}
