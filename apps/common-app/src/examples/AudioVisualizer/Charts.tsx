import React, { useMemo } from 'react';
import { Points, vec, Line } from '@shopify/react-native-skia';

import { useCanvas } from './Canvas';
import { colors } from '../../styles';
import { getPointCX, getPointCY } from '../DrumMachine/utils';

export function getAngle(stepIdx: number, maxSteps: number) {
  return (stepIdx / maxSteps) * Math.PI * 2 - Math.PI / 2;
}

interface ChartLineProps {
  data: number[];
  frequencyBinCount: number;
}

const TimeChartLine: React.FC<ChartLineProps> = (props) => {
  const { size } = useCanvas();
  const { data, frequencyBinCount } = props;

  const points = useMemo(() => {
    const maxHeight = size.height;
    const maxWidth = size.width;

    return data.map((value, index) => {
      const x = (index * maxWidth) / frequencyBinCount;
      const y = maxHeight - maxHeight * (value / 256) - 1;

      return vec(x, y);
    });
  }, [size, data, frequencyBinCount]);

  return (
    <Points
      points={points}
      mode="polygon"
      color={colors.main}
      strokeWidth={2}
    />
  );
};

const FrequencyChartLine: React.FC<ChartLineProps> = (props) => {
  const { size } = useCanvas();
  const { data, frequencyBinCount } = props;

  const points = useMemo(() => {
    const maxHeight = size.height;
    const maxWidth = size.width;
    const barWidth = maxWidth / frequencyBinCount;

    return data.map((value, index) => {
      const angle = getAngle(index, frequencyBinCount);
      const x = getPointCX(angle, 150, maxWidth / 2);
      const y = getPointCY(angle, 150, maxHeight / 2);
      // const x =  index / barWidth * 360;
      // const y = maxHeight - 64 * (value / 256);
      const x2 = getPointCX(angle, 150 + (Math.max(value, 10) / 256.0) * 30, maxWidth / 2);
      const y2 = getPointCY(angle, 150 + (Math.max(value, 10) / 256.0) * 30, maxHeight / 2);


      const hue = (index / frequencyBinCount) * 360;
      const color = `hsla(${hue}, 100%, 50%, 40%)`;
      return { color, x, y, x2, y2, barWidth };
    });
  }, [size, data, frequencyBinCount]);

  return (
    <>
      {points.map((point, index) => (
        <Line
          key={index}
          p1={vec(point.x, point.y)}
          p2={vec(point.x2, point.y2)}
          style="stroke"
          strokeWidth={point.barWidth}
          color={point.color}
        />
      ))}
    </>
  );
};

export default {
  TimeChartLine,
  FrequencyChartLine,
};
