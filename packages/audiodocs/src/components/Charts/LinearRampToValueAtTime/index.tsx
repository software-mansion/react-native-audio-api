import { Label, ReferenceLine } from "recharts";


import { FC } from "react";
import AudioParamChartBase from "../common/AudioParamChartBase";
import styles from '../styles.module.css';

const linearRampToValueAtTimeData = [
    { time: 0.0, value: 0.2 },
    { time: 0.1, value: 0.2},
    { time: 0.2, value: 0.2 },
    { time: 0.3, value: 0.3 },
    { time: 0.4, value: 0.4 },
    { time: 0.5, value: 0.5 },
    { time: 0.6, value: 0.6 },
    { time: 0.7, value: 0.7 },
    { time: 0.8, value: 0.8 },
    { time: 0.9, value: 0.8 },
];

const LinearRampToValueAtTimeChart: FC = () => (
    <AudioParamChartBase data={linearRampToValueAtTimeData}>
        <ReferenceLine x={0.2} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
            <Label value="startTime" position="bottom" className={styles.label} />
        </ReferenceLine>
        <ReferenceLine x={0.8} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
            <Label value="endTime" position="bottom" className={styles.label} />
        </ReferenceLine>
        <ReferenceLine y={0.2} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
            <Label value="previousValue" position="left" className={styles.label}/>
        </ReferenceLine>
        <ReferenceLine y={0.8} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
            <Label value="value" position="left" className={styles.label} />
        </ReferenceLine>
    </AudioParamChartBase>
);
 
export default LinearRampToValueAtTimeChart;