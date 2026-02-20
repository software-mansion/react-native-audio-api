import { LineChart, CartesianGrid, XAxis, YAxis, ReferenceLine, Label, Tooltip, Line} from "recharts";


import styles from '../styles.module.css';

// Generate exponential ramp data with more intermediate points
const generateExponentialRampData = () => {
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
  const steps = 60; // More granular steps for smooth curve
  for (let i = 0; i <= steps; i++) {
    const t = i / steps;
    const time = startTime + t * (endTime - startTime);
    // Exponential interpolation: value = startValue * (targetValue / startValue)^t
    const value = startValue * Math.pow(targetValue / startValue, t);
    data.push({ time: parseFloat(time.toFixed(3)), value: parseFloat(value.toFixed(3)) });
  }

  // Add points after the ramp
  for (let time = endTime + 0.1; time <= 0.9; time += 0.1) {
    data.push({ time: parseFloat(time.toFixed(1)), value: targetValue });
  }

  return data;
};

const exponentialRampToValueAtTimeData = generateExponentialRampData();

const ExponentialRampToValueAtTimeChart = () => (
    <LineChart
        className={styles.chart}
        responsive
        data={exponentialRampToValueAtTimeData}
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
            <Label value="previous event and time" position="bottom" className={styles.label} />
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
 
export default ExponentialRampToValueAtTimeChart;
