import { Label, ReferenceDot, ReferenceLine } from 'recharts';

import ChartBaseWrapper, { AudioParamDataPoint } from '../common/AudioParamChartBase';
import styles from '../styles.module.css';

const CustomLabelWithArrow = (props: any) => {
    const { viewBox, cx, cy } = props;
    
    // Use cx/cy if available (from ReferenceDot), otherwise fallback to viewBox
    const dotX = cx || viewBox?.cx || viewBox?.x || 0;
    const dotY = cy || viewBox?.cy || viewBox?.y || 0;
    
    // Position label below the dot
    const labelY = dotY + 30;
    const arrowStartY = labelY;
    const arrowEndY = dotY + 8;
    
    return (
        <g>
            {/* Arrow pointing from label to dot */}
            <defs>
                <marker
                    id="arrow"
                    markerWidth="10"
                    markerHeight="10"
                    refX="0"
                    refY="3"
                    orient="auto"
                    markerUnits="strokeWidth"
                >
                    <path d="M0,0 L0,6 L9,3 z" fill="currentColor" />
                </marker>
            </defs>
            <line
                x1={dotX}
                y1={arrowStartY}
                x2={dotX}
                y2={arrowEndY}
                stroke="currentColor"
                strokeWidth="1.5"
                markerEnd="url(#arrow)"
            />
            {/* Label text */}
            <text
                x={dotX}
                y={labelY + 20}
                textAnchor="middle"
                className={styles.referenceLabel}
                fill="currentColor"
            >
                timeConstant controls
            </text>
            <text
                x={dotX}
                y={labelY + 35}
                textAnchor="middle"
                className={styles.referenceLabel}
                fill="currentColor"
            >
                the change rate
            </text>
        </g>
    );
};

const generateSetTargetAtTimeData = (): AudioParamDataPoint[] => {
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
    <ChartBaseWrapper data={generateSetTargetAtTimeData()}>
    <ReferenceLine x={0.3} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="startTime" position="bottom" className={styles.label} />
    </ReferenceLine>
    <ReferenceLine y={0.2} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="previousValue" position="left" className={styles.label}/>
    </ReferenceLine>
    <ReferenceLine y={0.8} stroke='currentColor' className={styles.referenceLine} strokeDasharray="5 5">
        <Label value="target" position="left" className={styles.label} />
    </ReferenceLine>
    <ReferenceDot 
        x={0.65} 
        y={0.7} 
        r={4}
        fill="currentColor" 
        stroke="currentColor"
        strokeWidth={2}
        visibility={'hidden'}
        label={(props) => <CustomLabelWithArrow {...props} />}
    />
</ChartBaseWrapper>
);

export default SetTargetAtTimeChart;