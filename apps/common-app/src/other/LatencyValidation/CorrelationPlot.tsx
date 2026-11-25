import React, { FC, useMemo } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import Svg, { Circle, Line, Path } from 'react-native-svg';

import { Card } from '../../components';
import { colors, typography } from '../../styles';
import type { CorrelationPoint } from './types';

interface CorrelationPlotProps {
  title: string;
  caption?: string;
  width: number;
  height?: number;
  points: CorrelationPoint[];
  bestLagMs: number | null;
  expectedLagMs: number;
}

const CorrelationPlot: FC<CorrelationPlotProps> = ({
  title,
  caption,
  width,
  height = 120,
  points,
  bestLagMs,
  expectedLagMs,
}) => {
  const { path, bestX, expectedX, maxScore } = useMemo(() => {
    if (points.length === 0) {
      return { path: '', bestX: null, expectedX: null, maxScore: 1 };
    }

    const minLag = points[0].lagMs;
    const maxLag = points[points.length - 1].lagMs;
    const lagSpan = Math.max(maxLag - minLag, 1);
    const maxScoreValue = Math.max(
      ...points.map((point) => point.score),
      0.001
    );

    const builtPath = points
      .map((point, index) => {
        const x = ((point.lagMs - minLag) / lagSpan) * width;
        const y = height - (point.score / maxScoreValue) * (height * 0.82) - 8;
        return `${index === 0 ? 'M' : 'L'}${x.toFixed(2)},${y.toFixed(2)}`;
      })
      .join(' ');

    return {
      path: builtPath,
      bestX:
        bestLagMs !== null ? ((bestLagMs - minLag) / lagSpan) * width : null,
      expectedX: ((expectedLagMs - minLag) / lagSpan) * width,
      maxScore: maxScoreValue,
    };
  }, [bestLagMs, expectedLagMs, height, points, width]);

  return (
    <Card>
      <Text style={styles.title}>{title}</Text>
      {caption ? <Text style={typography.hint}>{caption}</Text> : null}

      <View style={[styles.plotFrame, { width, height }]}>
        <Svg width={width} height={height}>
          {expectedX !== null ? (
            <Line
              x1={expectedX}
              y1={0}
              x2={expectedX}
              y2={height}
              stroke={colors.yellow}
              strokeDasharray="4 4"
              strokeWidth={1}
            />
          ) : null}

          {bestX !== null ? (
            <Line
              x1={bestX}
              y1={0}
              x2={bestX}
              y2={height}
              stroke={colors.blue}
              strokeWidth={1.5}
            />
          ) : null}

          <Path d={path} stroke={colors.blue} strokeWidth={2} fill="none" />

          {bestX !== null ? (
            <Circle cx={bestX} cy={12} r={4} fill={colors.blue} />
          ) : null}
        </Svg>
      </View>

      <View style={styles.legendRow}>
        <Text style={styles.legendText}>
          Peak score max={maxScore.toFixed(3)}
        </Text>
        <Text style={[styles.legendText, { color: colors.blue }]}>
          Best lag {bestLagMs !== null ? `${bestLagMs.toFixed(1)} ms` : '—'}
        </Text>
        <Text style={[styles.legendText, { color: colors.yellow }]}>
          Expected {expectedLagMs.toFixed(1)} ms
        </Text>
      </View>
    </Card>
  );
};

const styles = StyleSheet.create({
  legendRow: {
    gap: 4,
  },
  legendText: {
    color: colors.textSecondary,
    fontSize: 11,
  },
  plotFrame: {
    backgroundColor: colors.recess,
    borderRadius: 12,
    overflow: 'hidden',
  },
  title: {
    color: colors.textPrimary,
    fontSize: 15,
    fontWeight: '700',
  },
});

export default CorrelationPlot;
