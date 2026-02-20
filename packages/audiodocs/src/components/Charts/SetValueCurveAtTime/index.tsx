import { LineChart, XAxis, YAxis, CartesianGrid, Tooltip, Line, ReferenceLine, Label, ReferenceDot } from 'recharts';

import styles from '../styles.module.css';


const setValueCurveAtTimeData = [
    { time: 0, value: 0.2 },
    { time: 0.1, value: 0.2},
    { time: 0.2, value: 0.2},
    { time: 0.2, value: 0.5},
    { time: 0.3, value: 0.35},
    { time: 0.4, value: 0.6},
    { time: 0.5, value: 0.45},
    { time: 0.6, value: 0.7},
    { time: 0.7, value: 0.7},
    { time: 0.8, value: 0.7},
    { time: 0.9, value: 0.7},
];

const SetValueCurveAtTimeChart = () => (
    <LineChart
        className={styles.chart}
        responsive
        data={setValueCurveAtTimeData}
        margin={{ left: 50, bottom: 20, top: 10 }}
    >
        <CartesianGrid strokeDasharray="3 3"/>
        <XAxis 
            dataKey="time"
            domain={[0, 0.9]}
            ticks={[]}
            tick={false}
            tickLine={true}
            type="number"
            label={{ value: 'Time', position: 'insideBottomRight', className: styles.label }}
            strokeWidth={2}
            stroke="currentColor"
    />
    <YAxis
        ticks={[]}
        tick={true}
        tickLine={true}
        domain={[0, 1.1]}
        label={{ value: 'Value', position: 'insideTopRight', offset: 10, className: styles.label }}
        strokeWidth={2}
        stroke="currentColor"
    />
    <ReferenceLine x={0.2} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="startTime" position="bottom" className={styles.label} />
    </ReferenceLine>
    <ReferenceLine x={0.6} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="startTime + duration" position="bottom" className={styles.label} />
    </ReferenceLine>
    <ReferenceLine y={0.2} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="previousValue" position="left" className={styles.label}/>
    </ReferenceLine>
    <ReferenceLine y={0.7} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5"/>
    <Line 
        type="linear" 
        dataKey="value"
        stroke='currentColor' 
        className={styles.leadingLine}
        strokeWidth={2}
        dot={false}
        activeDot={false}
        isAnimationActive={true}
    />
    <ReferenceDot x={0.2} y={0.5} className={styles.referenceDot} visibility={'hidden'} label={{ value: 'values[0]', position: 'top', className: styles.referenceLabel }}/>
    <ReferenceDot x={0.3} y={0.35} className={styles.referenceDot} visibility={'hidden'} label={{ value: 'values[1]', position: 'bottom', className: styles.referenceLabel }}/>
    <ReferenceDot x={0.4} y={0.6} className={styles.referenceDot} visibility={'hidden'} label={{ value: 'values[2]', position: 'top', className: styles.referenceLabel }}/>
    <ReferenceDot x={0.5} y={0.45} className={styles.referenceDot} visibility={'hidden'} label={{ value: 'values[3]', position: 'bottom', className: styles.referenceLabel }}/>
    <ReferenceDot x={0.6} y={0.7} className={styles.referenceDot} visibility={'hidden'} label={{ value: 'values[4]', position: 'top', className: styles.referenceLabel }}/>
    <Tooltip active={false} />
</LineChart>
);

export default SetValueCurveAtTimeChart;