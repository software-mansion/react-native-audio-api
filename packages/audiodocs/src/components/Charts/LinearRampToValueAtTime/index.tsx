import { LineChart, CartesianGrid, XAxis, YAxis, ReferenceLine, Label, Tooltip, Line} from "recharts";


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

const LinearRampToValueAtTimeChart = () => (
    <LineChart
        className={styles.chart}
        responsive
        data={linearRampToValueAtTimeData}
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
        <ReferenceLine x={0.8} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
            <Label value="endTime" position="bottom" className={styles.label} />
        </ReferenceLine>
        <ReferenceLine y={0.2} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
            <Label value="previousValue" position="left" className={styles.label}/>
        </ReferenceLine>
        <ReferenceLine y={0.8} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
            <Label value="value" position="left" className={styles.label} />
        </ReferenceLine>
        <Line 
            type="linear" 
            dataKey="value"
            stroke='currentColor' 
            className={styles.leadingLine}
            strokeWidth={2}
            dot={false}
            activeDot={false}
            isAnimationActive={false}
        />
        <Tooltip active={false} />
    </LineChart>
);
 
export default LinearRampToValueAtTimeChart;