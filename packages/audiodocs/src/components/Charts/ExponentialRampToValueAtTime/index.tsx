import {
    Label,
    ReferenceLine
} from 'recharts';

import { FC } from 'react';
import AudioParamChartBase, { AudioParamDataPoint } from '../common/AudioParamChartBase';
import styles from '../styles.module.css';

const generateExponentialRampData = (): AudioParamDataPoint[] => {
  const startTime = 0.2;
  const endTime = 0.8;
  const startValue = 0.2;
  const targetValue = 0.8;
  const data = [];

  // Add points before the ramp
  for (let time = 0.0; time < startTime; time += 0.1) {
    data.push({ time: parseFloat(time.toFixed(1)), value: startValue });
  }

  // Generate exponential ramp points
  const steps = 60;
  for (let i = 0; i <= steps; i++) {
    const t = i / steps;
    const time = startTime + t * (endTime - startTime);
    // Exponential interpolation: value = startValue * (targetValue / startValue)^t
    const value = startValue * Math.pow(targetValue / startValue, t);
    data.push({
      time: parseFloat(time.toFixed(3)),
      value: parseFloat(value.toFixed(3)),
    });
  }

  // Add points after the ramp
  for (let time = endTime + 0.1; time <= 0.9; time += 0.1) {
    data.push({ time: parseFloat(time.toFixed(1)), value: targetValue });
  }

  return data;
};

const ExponentialRampToValueAtTimeChart: FC = () => (
  <AudioParamChartBase data={generateExponentialRampData()}>
    <ReferenceLine
      x={0.2}
      stroke="currentColor"
      className={styles.referenceLine}
      strokeDasharray="5 5">
      <Label
        value="previous event and time"
        position="bottom"
        className={styles.label}
      />
    </ReferenceLine>
    <ReferenceLine
      x={0.8}
      stroke="currentColor"
      className={styles.referenceLine}
      strokeDasharray="5 5">
      <Label value="endTime" position="bottom" className={styles.label} />
    </ReferenceLine>
    <ReferenceLine
      y={0.2}
      stroke="currentColor"
      className={styles.referenceLine}
      strokeDasharray="5 5">
      <Label value="previousValue" position="left" className={styles.label} />
    </ReferenceLine>
    <ReferenceLine
      y={0.8}
      stroke="currentColor"
      className={styles.referenceLine}
      strokeDasharray="5 5">
      <Label value="value" position="left" className={styles.label} />
    </ReferenceLine>
  </AudioParamChartBase>
);

export default ExponentialRampToValueAtTimeChart;
