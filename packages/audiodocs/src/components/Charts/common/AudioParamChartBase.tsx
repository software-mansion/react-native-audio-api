import {
    CartesianGrid,
    Line,
    LineChart,
    Tooltip,
    XAxis,
    YAxis
} from 'recharts';

import { FC, ReactNode } from 'react';
import styles from '../styles.module.css';

export type AudioParamDataPoint = {
  time: number;
  value: number;
};

export interface AudioParamChartBaseProps {
  children: ReactNode;
  data: AudioParamDataPoint[];
}

const AudioParamChartBase: FC<AudioParamChartBaseProps> = ({
  children,
  data,
}) => (
  <LineChart
    className={styles.chart}
    responsive
    data={data}
    margin={{ left: 50, bottom: 20, top: 10 }}>
    <CartesianGrid strokeDasharray="3 3" />
    <XAxis
      dataKey="time"
      domain={[0, 0.9]}      
      ticks={[]}
      tick={false}
      tickLine={true}
      type="number"
      label={{
        value: 'Time',
        position: 'insideBottomRight',
        className: styles.label,
      }}
      strokeWidth={2}
      stroke="currentColor"
    />
    <YAxis
      ticks={[]}
      tick={true}
      tickLine={true}
      domain={[0, 1]}
      label={{
        value: 'Value',
        position: 'insideTopRight',
        offset: 10,
        className: styles.label,
      }}
      strokeWidth={2}
      stroke="currentColor"
    />
    <Line
      type="linear"
      dataKey="value"
      stroke="currentColor"
      className={styles.leadingLine}
      strokeWidth={2}
      dot={false}
      activeDot={false}
      isAnimationActive={false}
    />
    {children}
  </LineChart>
);

export default AudioParamChartBase;
