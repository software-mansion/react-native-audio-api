import {
  Picture,
  Skia,
  StrokeCap,
  StrokeJoin,
} from '@shopify/react-native-skia';
import { PaintStyle } from '@shopify/react-native-skia';
import React, { useMemo } from 'react';
import { SharedValue, useDerivedValue } from 'react-native-reanimated';

import { colors } from '../../styles';
import { useCanvas } from './Canvas';
import { getVisualizerLayout } from './layout';

/** Bars per half circle; the right half mirrors the left. */
const barsPerSide = 36;
/** Lowest and highest analyser bins the bars span, spaced logarithmically. */
const lowestBin = 2;
const highestBinShare = 0.8;
/**
 * Bars follow the signal level itself, as the original visualizer did: the
 * lower half of the bars is weighted down (from `bassWeight` at the lowest bar
 * up to 1 at the middle one), since music carries most of its energy there, and
 * the weighted level is expanded exponentially so loud passages stand out.
 */
const bassWeight = 0.7;
const levelExpansion = 2.5;
/** Weighted level (0–1, the analyser's byte range) that draws a full bar. */
const fullBarLevel = 0.95;
const quietBarOpacity = 0.35;
const loudBarOpacity = 0.95;

/** Points around the time-domain ring. */
const ringPoints = 128;
const ringOpacity = 0.7;

interface ChartProps {
  timeDataSV: SharedValue<Uint8Array>;
  timeDataTickSV: SharedValue<number>;
  frequencyDataSV: SharedValue<Uint8Array>;
  frequencyDataTickSV: SharedValue<number>;
  fftSize: number;
  frequencyBinCount: number;
}

function barWeight(bar: number) {
  'worklet';

  const weightedBars = barsPerSide / 2;
  return bar < weightedBars
    ? bassWeight + ((1 - bassWeight) * bar) / weightedBars
    : 1;
}

function expandLevel(level: number) {
  'worklet';

  return (
    (Math.exp(levelExpansion * level) - 1) /
    (Math.exp(levelExpansion * fullBarLevel) - 1)
  );
}

/**
 * First analyser bin of each bar, log-spaced so bass gets as many bars as
 * treble.
 */
function barBandEdges(frequencyBinCount: number) {
  const highestBin = Math.floor(frequencyBinCount * highestBinShare);
  const edges: number[] = [];
  for (let bar = 0; bar <= barsPerSide; bar += 1) {
    const bin = lowestBin * (highestBin / lowestBin) ** (bar / barsPerSide);
    edges.push(Math.round(bin));
  }
  return edges;
}

/**
 * Radial frequency bars in one accent: each bar's length and opacity follow its
 * band's loudest bin, and silence leaves a ring of dots (round caps on a
 * zero-length bar).
 */
const FrequencyChart: React.FC<ChartProps> = ({
  frequencyDataSV,
  frequencyDataTickSV,
  frequencyBinCount,
}) => {
  const { size } = useCanvas();
  const layout = useMemo(() => getVisualizerLayout(size.width), [size.width]);
  const edges = useMemo(
    () => barBandEdges(frequencyBinCount),
    [frequencyBinCount]
  );

  const paint = useMemo(() => {
    const barPaint = Skia.Paint();
    barPaint.setAntiAlias(true);
    barPaint.setStyle(PaintStyle.Stroke);
    barPaint.setStrokeCap(StrokeCap.Round);
    barPaint.setStrokeWidth(layout.barWidth);
    barPaint.setColor(Skia.Color(colors.blue));
    return barPaint;
  }, [layout.barWidth]);

  const picture = useDerivedValue(() => {
    frequencyDataTickSV.value;
    const data = frequencyDataSV.value;
    const { cx, cy, barRadius, barMaxLength } = layout;

    const recorder = Skia.PictureRecorder();
    const canvas = recorder.beginRecording(
      Skia.XYWHRect(0, 0, size.width, size.height)
    );

    for (let bar = 0; bar < barsPerSide; bar += 1) {
      let loudest = 0;
      const lastBin = Math.max(edges[bar], edges[bar + 1] - 1);
      for (let bin = edges[bar]; bin <= lastBin; bin += 1) {
        loudest = Math.max(loudest, data[bin]);
      }
      const level = Math.min(1, expandLevel((loudest / 255) * barWeight(bar)));
      const length = Math.max(0.01, level * barMaxLength);
      paint.setAlphaf(
        quietBarOpacity + (loudBarOpacity - quietBarOpacity) * level
      );

      // Bass at the top, treble meeting at the bottom, mirrored left and right.
      const offset = ((bar + 0.5) / barsPerSide) * Math.PI;
      for (const side of [-1, 1]) {
        const angle = -Math.PI / 2 + side * offset;
        const cos = Math.cos(angle);
        const sin = Math.sin(angle);
        canvas.drawLine(
          cx + cos * barRadius,
          cy + sin * barRadius,
          cx + cos * (barRadius + length),
          cy + sin * (barRadius + length),
          paint
        );
      }
    }

    return recorder.finishRecordingAsPicture();
  });

  return <Picture picture={picture} />;
};

/**
 * The waveform wrapped into a thin closed ring inside the bars: each point
 * moves in or out with the signal.
 */
const TimeChart: React.FC<ChartProps> = ({
  timeDataSV,
  timeDataTickSV,
  fftSize,
}) => {
  const { size } = useCanvas();
  const layout = useMemo(() => getVisualizerLayout(size.width), [size.width]);

  const paint = useMemo(() => {
    const ringPaint = Skia.Paint();
    ringPaint.setAntiAlias(true);
    ringPaint.setStyle(PaintStyle.Stroke);
    ringPaint.setStrokeJoin(StrokeJoin.Round);
    ringPaint.setStrokeWidth(layout.ringWidth);
    ringPaint.setColor(Skia.Color(colors.textPrimary));
    ringPaint.setAlphaf(ringOpacity);
    return ringPaint;
  }, [layout.ringWidth]);

  const picture = useDerivedValue(() => {
    timeDataTickSV.value;
    const data = timeDataSV.value;
    const { cx, cy, ringRadius, ringAmplitude } = layout;
    const samplesPerPoint = Math.max(1, Math.floor(fftSize / ringPoints));

    // Each point averages its samples, which keeps the ring smooth.
    const deviations: number[] = [];
    for (let point = 0; point < ringPoints; point += 1) {
      let sum = 0;
      for (let i = 0; i < samplesPerPoint; i += 1) {
        sum += data[point * samplesPerPoint + i] ?? 127.5;
      }
      deviations.push((sum / samplesPerPoint - 127.5) / 127.5);
    }
    // The waveform doesn't end where it starts; tilting it by the difference
    // closes the ring without a kink at the seam.
    const drift = deviations[ringPoints - 1] - deviations[0];
    for (let point = 0; point < ringPoints; point += 1) {
      deviations[point] -= (drift * point) / ringPoints;
    }

    const path = Skia.Path.Make();
    for (let point = 0; point < ringPoints; point += 1) {
      const angle = -Math.PI / 2 + (point / ringPoints) * 2 * Math.PI;
      const radius = ringRadius + deviations[point] * ringAmplitude;
      const x = cx + Math.cos(angle) * radius;
      const y = cy + Math.sin(angle) * radius;
      if (point === 0) {
        path.moveTo(x, y);
      } else {
        path.lineTo(x, y);
      }
    }
    path.close();

    const recorder = Skia.PictureRecorder();
    const canvas = recorder.beginRecording(
      Skia.XYWHRect(0, 0, size.width, size.height)
    );
    canvas.drawPath(path, paint);
    return recorder.finishRecordingAsPicture();
  });

  return <Picture picture={picture} />;
};

export default {
  TimeChart,
  FrequencyChart,
};
