import {
    CartesianGrid,
    Line,
    LineChart,
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
    <defs>
      <marker
        id="arrow-x"
        markerWidth="10"
        markerHeight="10"
        refX="8"
        refY="3"
        orient="auto"
        markerUnits="strokeWidth">
        <path d="M0,0 L0,6 L9,3 z" fill="currentColor" />
      </marker>
      <marker
        id="arrow-y"
        markerWidth="10"
        markerHeight="10"
        refX="8"
        refY="3"
        orient="270"
        markerUnits="strokeWidth">
        <path d="M0,0 L0,6 L9,3 z" fill="currentColor" />
      </marker>
    </defs>
    <CartesianGrid strokeDasharray="3 3" className={styles.leadingLine} stroke='currentColor'/>
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
      style={{ markerEnd: 'url(#arrow-x)' }}
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
      style={{ markerStart: 'url(#arrow-y)' }}
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
