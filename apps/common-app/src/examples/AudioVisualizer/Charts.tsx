import React, { useMemo } from 'react';
import { Circle, Path, Skia, PaintStyle } from '@shopify/react-native-skia';

import { useCanvas } from './Canvas';
import { colors } from '../../styles';
import { getAngle, getPointCX, getPointCY } from '../../utils/skiUtils';
import {
  getVisualizerLayout,
  sampleExp,
  weightWithIndex,
} from './layout';

interface ChartProps {
  timeData: Uint8Array;
  frequencyData: Uint8Array;
  fftSize: number;
  frequencyBinCount: number;
}

function buildTimePath(
  data: Uint8Array,
  fftSize: number,
  layout: ReturnType<typeof getVisualizerLayout>
) {
  const path = Skia.Path.Make();
  const { cx, cy, innerRadius } = layout;
  const span = innerRadius * 2;
  const startX = cx - innerRadius;

  for (let index = 0; index < data.length; index += 1) {
    const x = startX + (index / fftSize) * span;
    const y = cy - ((data[index] - 127) / 127) * innerRadius;

    if (index === 0) {
      path.moveTo(x, y);
    } else {
      path.lineTo(x, y);
    }
  }

  return path;
}

function buildFrequencyPath(
  data: Uint8Array,
  frequencyBinCount: number,
  layout: ReturnType<typeof getVisualizerLayout>
) {
  const path = Skia.Path.Make();
  const { cx, cy, outerRadius } = layout;
  const maxSteps = 2 * (frequencyBinCount - 64);

  for (let index = 0; index < data.length; index += 1) {
    if (index < 32 || index >= frequencyBinCount - 32) {
      continue;
    }

    const magnitude =
      sampleExp(weightWithIndex(data[index] / 255, index, frequencyBinCount)) *
      40;
    const indices = [index, 2 * frequencyBinCount - 65 - index];

    for (let side = 0; side < indices.length; side += 1) {
      const stepIndex = indices[side];
      const angle = getAngle(stepIndex - 32, maxSteps);
      const x1 = getPointCX(angle, outerRadius, cx);
      const y1 = getPointCY(angle, outerRadius, cy);
      const x2 = getPointCX(angle, outerRadius + magnitude, cx);
      const y2 = getPointCY(angle, outerRadius + magnitude, cy);

      path.moveTo(x1, y1);
      path.lineTo(x2, y2);
    }
  }

  return path;
}

const TimeChart: React.FC<ChartProps> = (props) => {
  const { size } = useCanvas();
  const { timeData, fftSize } = props;
  const layout = useMemo(
    () => getVisualizerLayout(size.width, size.height),
    [size.width, size.height]
  );

  const circlePaint = useMemo(() => {
    const paint = Skia.Paint();
    paint.setAntiAlias(true);
    paint.setColor(Skia.Color(colors.yellow));
    paint.setStyle(PaintStyle.Stroke);
    paint.setStrokeWidth(6);
    return paint;
  }, []);

  const timePath = useMemo(
    () => buildTimePath(timeData, fftSize, layout),
    [fftSize, layout, timeData]
  );

  return (
    <>
      <Circle
        cx={layout.cx}
        cy={layout.cy}
        r={layout.circleRadius}
        paint={circlePaint}
      />
      <Path
        path={timePath}
        color="#B5E1F1"
        style="stroke"
        strokeWidth={2}
        strokeJoin="round"
      />
    </>
  );
};

const FrequencyChart: React.FC<ChartProps> = (props) => {
  const { size } = useCanvas();
  const { frequencyData, frequencyBinCount } = props;
  const layout = useMemo(
    () => getVisualizerLayout(size.width, size.height),
    [size.width, size.height]
  );

  const barWidth = useMemo(
    () => (Math.PI * layout.outerRadius) / (frequencyBinCount - 64),
    [frequencyBinCount, layout.outerRadius]
  );

  const freqPath = useMemo(
    () => buildFrequencyPath(frequencyData, frequencyBinCount, layout),
    [frequencyBinCount, frequencyData, layout]
  );

  return (
    <Path
      path={freqPath}
      color="rgba(79, 195, 247, 0.85)"
      style="stroke"
      strokeWidth={barWidth}
      strokeCap="round"
    />
  );
};

export default {
  TimeChart,
  FrequencyChart,
};
