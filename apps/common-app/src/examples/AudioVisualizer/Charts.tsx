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
    const maxHeight = size.height / 2;
    const maxWidth = size.width / 2;

    return data.map((value, index) => {
      const x = size.width / 4 + (index * maxWidth) / (frequencyBinCount * 2);
      const y = size.height / 4 + maxHeight - maxHeight * (value / 256) - 1;

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
    const barWidth = (Math.PI * 150) / (frequencyBinCount - 64);

    function getRect(index: number, value: number) {
      const angle = getAngle(index, (frequencyBinCount - 64) * 2);
      const x1 = getPointCX(angle, 150, maxWidth / 2);
      const y1 = getPointCY(angle, 150, maxHeight / 2);
      // const x =  index / barWidth * 360;
      // const y = maxHeight - 64 * (value / 256);
      const x2 = getPointCX(angle, 150 + (Math.max(value, 10) / 256.0) * 30, maxWidth / 2);
      const y2 = getPointCY(angle, 150 + (Math.max(value, 10) / 256.0) * 30, maxHeight / 2);

      return {
        x1,
        y1,
        x2,
        y2,
      };
    }

    const points = [];

    data.forEach((value, index) => {
      if (index < 32 || index >= frequencyBinCount - 32) {
        return;
      }

      const hue = (index / (frequencyBinCount - 64)) * 360;
      const color = `hsla(${hue}, 100%, 50%, 40%)`;

      points.push({
        ...getRect(index, value),
        color,
        barWidth,
      });

      points.push({
        ...getRect(2 * frequencyBinCount - 65 - index, value),
        color,
        barWidth,
      });
    });

    return points;
  }, [size, data, frequencyBinCount]);

  return (
    <>
      {points.map((point, index) => (
        <Line
          key={index}
          p1={vec(point.x1, point.y1)}
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
