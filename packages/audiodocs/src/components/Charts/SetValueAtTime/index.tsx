import { Label, Line, ReferenceLine, Tooltip } from 'recharts';

import { FC } from 'react';
import AudioParamChartBase from '../common/AudioParamChartBase';
import styles from '../styles.module.css';

const setValueAtTimeData = [
  { time: 0.0, value: 0.2 },
  { time: 0.1, value: 0.2 },
  { time: 0.2, value: 0.2 },
  { time: 0.3, value: 0.2 },
  { time: 0.3, value: 0.8 },
  { time: 0.4, value: 0.8 },
  { time: 0.5, value: 0.8 },
  { time: 0.6, value: 0.8 },
  { time: 0.7, value: 0.8 },
  { time: 0.8, value: 0.8 },
  { time: 0.9, value: 0.8 },
];

const SetValueAtTimeChart: FC = () => (
<AudioParamChartBase data={setValueAtTimeData}>
    <ReferenceLine x={0.3} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="startTime" position="bottom" className={styles.label} />
    </ReferenceLine>
    <ReferenceLine y={0.2} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="previousValue" position="left" className={styles.label}/>
    </ReferenceLine>
    <ReferenceLine y={0.8} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="value" position="left" className={styles.label} />
    </ReferenceLine>
    <Line 
        type="stepAfter" 
        dataKey="value"
        stroke='currentColor' 
        className={styles.leadingLine}
        strokeWidth={2}
        dot={false}
        activeDot={false}
        isAnimationActive={true}
    />
    <Tooltip active={false} />
</AudioParamChartBase>
);

export default  SetValueAtTimeChart;