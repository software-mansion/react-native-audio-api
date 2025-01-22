import React, { useMemo } from 'react';
import { Points, vec, Rect } from '@shopify/react-native-skia';

import { useCanvas } from './Canvas';
import { colors } from '../../styles';

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
      const x = index * barWidth;
      const y = maxHeight - maxHeight * (value / 256);
      const hue = (index / frequencyBinCount) * 360;
      const color = `hsl(${hue}, 100%, 50%)`;
      return { x, y, height: maxHeight - y, color, width: barWidth };
    });
  }, [size, data, frequencyBinCount]);

  return (
    <>
      {points.map((point, index) => (
        <Rect
          key={index}
          x={point.x}
          y={point.y}
          width={point.width}
          height={point.height}
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
