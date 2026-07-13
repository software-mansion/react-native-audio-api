import { FC, useMemo } from 'react';
import { Line, LineChart, ReferenceLine, XAxis, YAxis } from 'recharts';

import styles from './styles.module.css';

type WaveType = 'sine' | 'square' | 'sawtooth' | 'triangle';

type WavePoint = { x: number; y: number };

interface WaveConfig {
  type: WaveType;
  label: string;
  // References a theme-aware CSS variable defined in src/css/colors.css so the
  // colors track the documentation palette (and light/dark mode) automatically.
  colorVar: string;
}

const CYCLES = 3;
const SINE_SAMPLES = 128;

// Vertical grid lines at multiples of π. Every waveform shares the same phase
// axis where one full period spans 2π, so a multiple of π falls on each
// half-cycle boundary: x = k / (2 * CYCLES).
const PI_MULTIPLE_LINES = Array.from(
  { length: 2 * CYCLES - 1 },
  (_, i) => (i + 1) / (2 * CYCLES)
);

const HORIZONTAL_GRID_LINES = [1.0, 0.0, -1.0];

const WAVES: WaveConfig[] = [
  { type: 'sine', label: 'Sine', colorVar: 'var(--swm-oscillator-sine)' },
  { type: 'square', label: 'Square', colorVar: 'var(--swm-oscillator-square)' },
  { type: 'sawtooth', label: 'Sawtooth', colorVar: 'var(--swm-oscillator-sawtooth)' },
  { type: 'triangle', label: 'Triangle', colorVar: 'var(--swm-oscillator-triangle)' },
];

function generateWave(type: WaveType): WavePoint[] {
  const cycleWidth = 1 / CYCLES;

  if (type === 'sine') {
    return Array.from({ length: SINE_SAMPLES + 1 }, (_, i) => {
      const x = i / SINE_SAMPLES;
      return { x, y: Math.sin(2 * Math.PI * CYCLES * x) };
    });
  }

  const points: WavePoint[] = [];

  for (let c = 0; c < CYCLES; c += 1) {
    const base = c * cycleWidth;

    switch (type) {
      case 'square':
        points.push({ x: base, y: 1 });
        points.push({ x: base + cycleWidth / 2, y: 1 });
        points.push({ x: base + cycleWidth / 2, y: -1 });
        points.push({ x: base + cycleWidth, y: -1 });
        break;
      case 'sawtooth':
        // Web Audio synthesizes waveforms as a sum of sines (odd function,
        // zero DC), so the sawtooth starts at 0, ramps to +1, drops to -1 at
        // the middle of each period, then ramps back to 0. Matches the engine's
        // definition in PeriodicWave.cpp: f(x) = 2 * (t - floor(t + 0.5)).
        points.push({ x: base, y: 0 });
        points.push({ x: base + cycleWidth / 2, y: 1 });
        points.push({ x: base + cycleWidth / 2, y: -1 });
        points.push({ x: base + cycleWidth, y: 0 });
        break;
      case 'triangle':
        points.push({ x: base, y: 0 });
        points.push({ x: base + cycleWidth * 0.25, y: 1 });
        points.push({ x: base + cycleWidth * 0.75, y: -1 });
        points.push({ x: base + cycleWidth, y: 0 });
        break;
    }
  }

  return points;
}

interface WaveChartProps {
  config: WaveConfig;
  data: WavePoint[];
}

// The panel's `color` is set to the wave's palette variable, so the wave line
// (stroke="currentColor") and the label both inherit it. Grid lines are colored
// independently via CSS in styles.module.css.
const WaveChart: FC<WaveChartProps> = ({ config, data }) => (
  <div className={styles.panel} style={{ color: config.colorVar }}>
    <span className={styles.label}>{config.label}</span>
    <LineChart
      className={styles.chart}
      responsive
      data={data}
      margin={{ top: 8, right: 8, bottom: 8, left: 8 }}>
      {HORIZONTAL_GRID_LINES.map((y) => (
        <ReferenceLine key={`h-${y}`} y={y} strokeDasharray="4 4" />
      ))}
      {PI_MULTIPLE_LINES.map((x) => (
        <ReferenceLine key={`v-${x}`} x={x} strokeDasharray="4 4" />
      ))}
      <XAxis
        dataKey="x"
        type="number"
        domain={[0, 1]}
        tickCount={9}
        tick={false}
        tickLine={false}
        axisLine={false}
      />
      <YAxis
        type="number"
        domain={[-1.2, 1.2]}
        tickCount={5}
        tick={false}
        tickLine={false}
        axisLine={false}
      />
      <Line
        type="linear"
        dataKey="y"
        stroke="currentColor"
        strokeWidth={2}
        dot={false}
        activeDot={false}
        isAnimationActive={false}
      />
    </LineChart>
  </div>
);

const OscillatorWavesChart: FC = () => {
  const waves = useMemo(
    () =>
      WAVES.map((config) => ({
        config,
        data: generateWave(config.type),
      })),
    []
  );

  return (
    <div className={styles.container}>
      {waves.map(({ config, data }) => (
        <WaveChart key={config.type} config={config} data={data} />
      ))}
    </div>
  );
};

export default OscillatorWavesChart;
