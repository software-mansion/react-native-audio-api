import { Label, ReferenceDot, ReferenceLine } from 'recharts';

import AudioParamChartBase, { AudioParamChartBaseProps } from '../common/AudioParamChartBase';
import styles from '../styles.module.css';
import { FC } from 'react';


const setValueCurveAtTimeData = [
    { time: 0, value: 0.2 },
    { time: 0.1, value: 0.2},
    { time: 0.2, value: 0.2},
    { time: 0.2, value: 0.5},
    { time: 0.3, value: 0.35},
    { time: 0.4, value: 0.6},
    { time: 0.5, value: 0.45},
    { time: 0.6, value: 0.8},
    { time: 0.7, value: 0.8},
    { time: 0.8, value: 0.8},
    { time: 0.9, value: 0.8},
];

const SetValueCurveAtTimeChart: FC = () => (
    <AudioParamChartBase data={setValueCurveAtTimeData}>
    <ReferenceLine x={0.2} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="startTime" position="bottom" className={styles.label} />
    </ReferenceLine>
    <ReferenceLine x={0.6} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="startTime + duration" position="bottom" className={styles.label} />
    </ReferenceLine>
    <ReferenceLine y={0.2} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="previousValue" position="left" className={styles.label}/>
    </ReferenceLine>
    <ReferenceLine y={0.8} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5"/>
    <ReferenceDot x={0.2} y={0.5} className={styles.referenceDot} visibility={'hidden'} label={{ value: 'values[0]', position: 'top', className: styles.label }}/>
    <ReferenceDot x={0.3} y={0.35} className={styles.referenceDot} visibility={'hidden'} label={{ value: 'values[1]', position: 'bottom', className: styles.label }}/>
    <ReferenceDot x={0.4} y={0.6} className={styles.referenceDot} visibility={'hidden'} label={{ value: 'values[2]', position: 'top', className: styles.label }}/>
    <ReferenceDot x={0.5} y={0.45} className={styles.referenceDot} visibility={'hidden'} label={{ value: 'values[3]', position: 'bottom', className: styles.label }}/>
    <ReferenceDot x={0.6} y={0.8} className={styles.referenceDot} visibility={'hidden'} label={{ value: 'values[4]', position: 'top', className: styles.label }}/>
</AudioParamChartBase>
);

export default SetValueCurveAtTimeChart;