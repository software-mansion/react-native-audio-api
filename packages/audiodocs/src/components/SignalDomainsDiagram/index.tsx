import clsx from 'clsx';
import React, { FC, useId } from 'react';

import styles from './styles.module.css';

interface SineComponent {
  cycles: number;
  amplitude: number;
  /** Position along the frequency axis of the decomposition view, 0 (front) to 1 (back). */
  depth: number;
}

const LOW_COMPONENT: SineComponent = { cycles: 2, amplitude: 0.55, depth: 0.12 };
const HIGH_COMPONENT: SineComponent = { cycles: 5, amplitude: 0.3, depth: 0.62 };
const SAMPLES = 120;

type Point = [number, number];
type PointMapper = (time: number, amplitude: number) => Point;

function toPath(points: Point[]): string {
  return points.map(([x, y], index) => `${index === 0 ? 'M' : 'L'} ${x.toFixed(1)} ${y.toFixed(1)}`).join(' ');
}

function sine(component: SineComponent, time: number): number {
  return component.amplitude * Math.sin(2 * Math.PI * component.cycles * time);
}

function tracePath(valueAt: (time: number) => number, mapPoint: PointMapper): string {
  const points: Point[] = [];
  for (let i = 0; i <= SAMPLES; i++) {
    const time = i / SAMPLES;
    points.push(mapPoint(time, valueAt(time)));
  }
  return toPath(points);
}

const PANEL_WIDTH = 200;
const PANEL_HEIGHT = 150;
const PANEL_PLOT_LEFT = 30;
const PANEL_PLOT_RIGHT = 190;
const PANEL_AXIS_Y = 128;
const PANEL_AXIS_TOP = 10;
const PANEL_BASELINE_Y = 72;
const PANEL_AMPLITUDE_SCALE = 42;
const PANEL_BAR_SCALE = 84;
const PANEL_BAR_WIDTH = 7;

const VIEW_WIDTH = 280;
const VIEW_HEIGHT = 200;
const VIEW_ORIGIN: Point = [44, 150];
const VIEW_TIME_LENGTH = 140;
const VIEW_AMPLITUDE_SCALE = 34;
/** Screen offset of one unit along the frequency axis, drawn receding up and to the right. */
const VIEW_DEPTH: Point = [80, -58];

const ArrowMarkers: FC<{ id: string }> = ({ id }) => (
  <defs>
    <marker id={id} markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto">
      <path className={styles.arrowHead} d="M0 0L8 4L0 8z" />
    </marker>
  </defs>
);

interface PanelAxesProps {
  markerId: string;
  horizontalLabel: string;
  verticalLabel: string;
}

const PanelAxes: FC<PanelAxesProps> = ({ markerId, horizontalLabel, verticalLabel }) => (
  <g>
    <line
      className={styles.axis}
      x1={PANEL_PLOT_LEFT}
      y1={PANEL_AXIS_Y}
      x2={PANEL_PLOT_RIGHT + 6}
      y2={PANEL_AXIS_Y}
      markerEnd={`url(#${markerId})`}
    />
    <line
      className={styles.axis}
      x1={PANEL_PLOT_LEFT}
      y1={PANEL_AXIS_Y}
      x2={PANEL_PLOT_LEFT}
      y2={PANEL_AXIS_TOP}
      markerEnd={`url(#${markerId})`}
    />
    <text className={styles.axisLabel} x={PANEL_PLOT_RIGHT + 6} y={PANEL_AXIS_Y + 14} textAnchor="end">
      {horizontalLabel}
    </text>
    <text
      className={styles.axisLabel}
      x={12}
      y={(PANEL_AXIS_TOP + PANEL_AXIS_Y) / 2}
      textAnchor="middle"
      transform={`rotate(-90 12 ${(PANEL_AXIS_TOP + PANEL_AXIS_Y) / 2})`}>
      {verticalLabel}
    </text>
  </g>
);

const TimeDomainPanel: FC = () => {
  const markerId = useId();
  const mapPoint: PointMapper = (time, amplitude) => [
    PANEL_PLOT_LEFT + time * (PANEL_PLOT_RIGHT - PANEL_PLOT_LEFT),
    PANEL_BASELINE_Y - amplitude * PANEL_AMPLITUDE_SCALE,
  ];

  return (
    <svg className={styles.svg} viewBox={`0 0 ${PANEL_WIDTH} ${PANEL_HEIGHT}`} style={{ maxWidth: PANEL_WIDTH }} role="img" aria-label="Time domain: amplitude of the signal over time, with its two sine components drawn underneath the summed waveform">
      <ArrowMarkers id={markerId} />
      <PanelAxes markerId={markerId} horizontalLabel="time" verticalLabel="amplitude" />
      <path className={clsx(styles.wave, styles.componentWave, styles.low)} d={tracePath((t) => sine(LOW_COMPONENT, t), mapPoint)} />
      <path className={clsx(styles.wave, styles.componentWave, styles.high)} d={tracePath((t) => sine(HIGH_COMPONENT, t), mapPoint)} />
      <path className={clsx(styles.wave, styles.sum)} d={tracePath((t) => sine(LOW_COMPONENT, t) + sine(HIGH_COMPONENT, t), mapPoint)} />
    </svg>
  );
};

const FrequencyDomainPanel: FC = () => {
  const markerId = useId();
  const plotWidth = PANEL_PLOT_RIGHT - PANEL_PLOT_LEFT;

  const bar = (component: SineComponent, className: string) => {
    const x = PANEL_PLOT_LEFT + 0.22 + component.depth * plotWidth * 0.8 + plotWidth * 0.1;
    const height = component.amplitude * PANEL_BAR_SCALE;
    return (
      <rect
        className={clsx(styles.bar, className)}
        x={x - PANEL_BAR_WIDTH / 2}
        y={PANEL_AXIS_Y - height}
        width={PANEL_BAR_WIDTH}
        height={height}
        rx={2}
      />
    );
  };

  return (
    <svg className={styles.svg} viewBox={`0 0 ${PANEL_WIDTH} ${PANEL_HEIGHT}`} style={{ maxWidth: PANEL_WIDTH }} role="img" aria-label="Frequency domain: one bar per sine component, placed at its frequency and as tall as its amplitude">
      <ArrowMarkers id={markerId} />
      <PanelAxes markerId={markerId} horizontalLabel="frequency" verticalLabel="magnitude" />
      {bar(LOW_COMPONENT, styles.low)}
      {bar(HIGH_COMPONENT, styles.high)}
    </svg>
  );
};

const DecompositionView: FC = () => {
  const markerId = useId();
  const [originX, originY] = VIEW_ORIGIN;
  const [depthX, depthY] = VIEW_DEPTH;

  const mapAt = (depth: number): PointMapper => (time, amplitude) => [
    originX + time * VIEW_TIME_LENGTH + depth * depthX,
    originY - amplitude * VIEW_AMPLITUDE_SCALE + depth * depthY,
  ];

  const componentWave = (component: SineComponent, className: string) => {
    const mapPoint = mapAt(component.depth);
    const [startX, startY] = mapPoint(0, 0);
    const [endX, endY] = mapPoint(1, 0);
    return (
      <g>
        <line className={styles.zeroLine} x1={startX} y1={startY} x2={endX} y2={endY} />
        <circle className={clsx(styles.marker, className)} cx={originX + component.depth * depthX} cy={originY + component.depth * depthY} r={3} />
        <path className={clsx(styles.wave, className)} d={tracePath((t) => sine(component, t), mapPoint)} />
      </g>
    );
  };

  const frequencyAxisEnd: Point = [originX + depthX * 1.1, originY + depthY * 1.1];

  return (
    <svg className={styles.svg} viewBox={`0 0 ${VIEW_WIDTH} ${VIEW_HEIGHT}`} style={{ maxWidth: VIEW_WIDTH }} role="img" aria-label="The same signal split into two sine waves, each drawn at its own position along a frequency axis that recedes into the picture">
      <ArrowMarkers id={markerId} />
      <line className={styles.axis} x1={originX} y1={originY} x2={originX + VIEW_TIME_LENGTH + 40} y2={originY} markerEnd={`url(#${markerId})`} />
      <line className={styles.axis} x1={originX} y1={originY} x2={originX} y2={originY - 118} markerEnd={`url(#${markerId})`} />
      <line className={styles.axis} x1={originX} y1={originY} x2={frequencyAxisEnd[0]} y2={frequencyAxisEnd[1]} markerEnd={`url(#${markerId})`} />
      <text className={styles.axisLabel} x={originX + VIEW_TIME_LENGTH + 40} y={originY + 14} textAnchor="end">
        time
      </text>
      <text className={styles.axisLabel} x={originX} y={originY - 124} textAnchor="middle">
        amplitude
      </text>
      <text className={styles.axisLabel} x={frequencyAxisEnd[0] + 6} y={frequencyAxisEnd[1] + 4} textAnchor="start">
        frequency
      </text>
      {componentWave(HIGH_COMPONENT, styles.high)}
      {componentWave(LOW_COMPONENT, styles.low)}
    </svg>
  );
};

/**
 * Time domain versus frequency domain: the summed waveform on the left, its sine components pulled
 * apart along a frequency axis in the middle, and the resulting spectrum on the right.
 */
const SignalDomainsDiagram: FC = () => (
  <div className={styles.row}>
    <figure className={styles.figure}>
      <TimeDomainPanel />
      <figcaption className={styles.caption}>
        Time domain
        <span className={styles.captionDetail}>
          <code>getFloatTimeDomainData()</code>
        </span>
      </figcaption>
    </figure>
    <figure className={clsx(styles.figure, styles.figureWide)}>
      <DecompositionView />
      <figcaption className={styles.caption}>
        Same signal, components apart
        <span className={styles.captionDetail}>each sine sits at its own frequency</span>
      </figcaption>
    </figure>
    <figure className={styles.figure}>
      <FrequencyDomainPanel />
      <figcaption className={styles.caption}>
        Frequency domain
        <span className={styles.captionDetail}>
          <code>getFloatFrequencyData()</code>
        </span>
      </figcaption>
    </figure>
  </div>
);

export default SignalDomainsDiagram;
