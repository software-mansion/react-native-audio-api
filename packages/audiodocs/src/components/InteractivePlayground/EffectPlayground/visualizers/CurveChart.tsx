import { useColorMode } from '@docusaurus/theme-common';
import React, { FC, useCallback } from 'react';

import ResponsiveCanvas, { ResponsiveCanvasDrawParams } from '@site/src/ui/ResponsiveCanvas';

const DEFAULT_HEIGHT = 120;
const PADDING = 6;

interface CurveChartProps {
  /** Samples spread evenly across the width; `values[0]` is drawn at the left edge. */
  values: ArrayLike<number>;
  yMin?: number;
  yMax?: number;
  /** Draws the y = x diagonal for transfer curves so the distortion is visible as a deviation. */
  showIdentity?: boolean;
  height?: number;
}

const CurveChart: FC<CurveChartProps> = ({
  values,
  yMin = -1,
  yMax = 1,
  showIdentity = false,
  height = DEFAULT_HEIGHT,
}) => {
  const { colorMode } = useColorMode();

  const handleCanvasDraw = useCallback(
    ({ context, cssWidth, cssHeight, dpr }: ResponsiveCanvasDrawParams) => {
      const isDark = colorMode === 'dark';
      const plotWidth = cssWidth - PADDING * 2;
      const plotHeight = cssHeight - PADDING * 2;
      const yToCanvas = (value: number) => PADDING + (1 - (value - yMin) / (yMax - yMin)) * plotHeight;

      context.setTransform(dpr, 0, 0, dpr, 0, 0);
      context.clearRect(0, 0, cssWidth, cssHeight);

      context.strokeStyle = isDark ? 'rgba(252, 252, 255, 0.15)' : 'rgba(0, 0, 0, 0.15)';
      context.lineWidth = 1;
      context.strokeRect(PADDING + 0.5, PADDING + 0.5, plotWidth - 1, plotHeight - 1);

      if (yMin < 0 && yMax > 0) {
        const zeroY = Math.round(yToCanvas(0)) + 0.5;
        context.beginPath();
        context.moveTo(PADDING, zeroY);
        context.lineTo(PADDING + plotWidth, zeroY);
        context.stroke();
        const midX = Math.round(PADDING + plotWidth / 2) + 0.5;
        context.beginPath();
        context.moveTo(midX, PADDING);
        context.lineTo(midX, PADDING + plotHeight);
        context.stroke();
      }

      if (showIdentity) {
        context.setLineDash([4, 4]);
        context.strokeStyle = isDark ? 'rgba(252, 252, 255, 0.35)' : 'rgba(0, 0, 0, 0.3)';
        context.beginPath();
        context.moveTo(PADDING, yToCanvas(yMin));
        context.lineTo(PADDING + plotWidth, yToCanvas(yMax));
        context.stroke();
        context.setLineDash([]);
      }

      const count = values.length;
      if (count < 2) {
        return;
      }

      context.beginPath();
      for (let i = 0; i < count; i++) {
        const x = PADDING + (i / (count - 1)) * plotWidth;
        const y = yToCanvas(Math.min(yMax, Math.max(yMin, values[i])));
        if (i === 0) {
          context.moveTo(x, y);
        } else {
          context.lineTo(x, y);
        }
      }
      context.strokeStyle = isDark ? '#ff7774' : '#fa7f7c';
      context.lineWidth = 2;
      context.stroke();
    },
    [colorMode, showIdentity, values, yMax, yMin]
  );

  return <ResponsiveCanvas onDraw={handleCanvasDraw} height={height} />;
};

export default CurveChart;
