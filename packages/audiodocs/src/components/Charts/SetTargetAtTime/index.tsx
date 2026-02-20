import { LineChart, XAxis, YAxis, CartesianGrid, Tooltip, Line, ReferenceLine, Label } from 'recharts';

import styles from '../styles.module.css';

const generateSetTargetAtTimeData = () => {
    const startTime = 0.3;
    const timeConstant = 0.1;
    const previousValue = 0.2;
    const targetValue = 0.8;
    const data = [];

    // Add points before the change
    for (let time = 0.0; time < startTime; time += 0.1) {
        data.push({ time: parseFloat(time.toFixed(1)), value: previousValue });
    }

    // Generate points after the change using the formula
    const steps = 60; // More granular steps for smooth curve

    for (let i = 0; i < steps; i++) {
        const t = i / steps;
        const time = startTime + t * (0.9 - startTime); // Spread points until 0.9
        const value = targetValue + (previousValue - targetValue) * Math.exp(-(time - startTime) / timeConstant);
        data.push({ time: parseFloat(time.toFixed(3)), value: parseFloat(value.toFixed(3)) });
    }

    return data;
}

const SetTargetAtTimeChart = () => (
    <LineChart
        className={styles.chart}
        responsive
        data={generateSetTargetAtTimeData()}
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
    <ReferenceLine x={0.3} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="startTime" position="bottom" className={styles.label} />
    </ReferenceLine>
    <ReferenceLine y={0.2} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="previousValue" position="left" className={styles.label}/>
    </ReferenceLine>
    <ReferenceLine y={0.8} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="target" position="left" className={styles.label} />
    </ReferenceLine>
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
    <Tooltip active={false} />
</LineChart>
);

export default  SetTargetAtTimeChart;