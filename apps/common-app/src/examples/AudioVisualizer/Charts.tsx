import React, { useMemo } from 'react';
import {
  Points,
  vec,
  Line,
  Circle,
  Skia,
  PaintStyle,
} from '@shopify/react-native-skia';

import { useCanvas } from './Canvas';
import { colors } from '../../styles';
import { getPointCX, getPointCY } from '../DrumMachine/utils';

const INNER_RADIUS = 120;
const OUTER_RADIUS = 150;

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

  const circlePaint = useMemo(() => {
    const paint = Skia.Paint();
    paint.setAntiAlias(true);
    paint.setColor(Skia.Color(colors.main));
    paint.setStyle(PaintStyle.Stroke);
    paint.setStrokeWidth(3);
    return paint;
  }, []);

  const points = useMemo(() => {
    const maxHeight = size.height;
    const maxWidth = size.width;

    const startWidth = (maxWidth - 2 * INNER_RADIUS) / 2;
    const startHight = maxHeight - (maxHeight - 2 * INNER_RADIUS) / 2;

    return data.map((value, index) => {
      const x = startWidth + (index * 2 * INNER_RADIUS) / frequencyBinCount;
      const y = startHight - (value / 256) * 2 * INNER_RADIUS;

      return vec(x, y);
    });
  }, [size, data, frequencyBinCount]);

  return (
    <>
      <Circle
        cx={size.width / 2}
        cy={size.height / 2}
        r={INNER_RADIUS * 1.1}
        paint={circlePaint}
      />
      <Points
        points={points}
        mode="polygon"
        color={colors.main}
        strokeWidth={2}
      />
    </>
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
      const x = getPointCX(angle, OUTER_RADIUS, maxWidth / 2);
      const y = getPointCY(angle, OUTER_RADIUS, maxHeight / 2);

      const x2 = getPointCX(
        angle,
        OUTER_RADIUS + (Math.max(value, 10) / 256.0) * 30,
        maxWidth / 2
      );
      const y2 = getPointCY(
        angle,
        OUTER_RADIUS + (Math.max(value, 10) / 256.0) * 30,
        maxHeight / 2
      );

      const hue = 180 + (index / frequencyBinCount) * 60;
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
