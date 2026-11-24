import React, { FC, useMemo } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import Svg, { Line, Path } from 'react-native-svg';

import { Card } from '../../components';
import { colors, typography } from '../../styles';
import type { WaveformSeries } from './types';

interface PlotMarker {
  ratio: number;
  label: string;
  color: string;
}

interface WaveformPlotProps {
  title: string;
  caption?: string;
  width: number;
  height?: number;
  series: WaveformSeries[];
  markers?: PlotMarker[];
  xAxisLabel?: string;
  sharedScale?: boolean;
}

function buildEnvelopePath(
  points: number[],
  width: number,
  height: number,
  scaleMax: number
): string {
  if (points.length === 0) {
    return '';
  }

  return points
    .map((point, index) => {
      const x = points.length <= 1 ? 0 : (index / (points.length - 1)) * width;
      const y = height - (Math.abs(point) / scaleMax) * (height * 0.82) - 6;
      return `${index === 0 ? 'M' : 'L'}${x.toFixed(2)},${y.toFixed(2)}`;
    })
    .join(' ');
}

const WaveformPlot: FC<WaveformPlotProps> = ({
  title,
  caption,
  width,
  height = 120,
  series,
  markers = [],
  xAxisLabel,
  sharedScale = false,
}) => {
  const paths = useMemo(() => {
    const peak = (points: number[]) =>
      Math.max(...points.map((point) => Math.abs(point)), 0.001);
    const sharedMax = Math.max(...series.map((entry) => peak(entry.points)));

    return series.map((entry) => ({
      ...entry,
      path: buildEnvelopePath(
        entry.points,
        width,
        height,
        sharedScale ? sharedMax : peak(entry.points)
      ),
    }));
  }, [height, series, sharedScale, width]);

  return (
    <Card>
      <Text style={styles.title}>{title}</Text>
      {caption ? <Text style={typography.hint}>{caption}</Text> : null}

      <View style={[styles.plotFrame, { width, height }]}>
        <Svg width={width} height={height}>
          <Line
            x1={0}
            y1={height - 6}
            x2={width}
            y2={height - 6}
            stroke={colors.track}
            strokeWidth={1}
          />

          {markers.map((marker) => (
            <Line
              key={`${marker.label}-${marker.ratio}`}
              x1={marker.ratio * width}
              y1={0}
              x2={marker.ratio * width}
              y2={height}
              stroke={marker.color}
              strokeDasharray="4 4"
              strokeWidth={1}
            />
          ))}

          {paths.map((entry) => (
            <Path
              key={entry.label}
              d={entry.path}
              stroke={entry.color}
              strokeWidth={2}
              fill="none"
            />
          ))}
        </Svg>
      </View>

      <View style={styles.legendRow}>
        {series.map((entry) => (
          <View key={entry.label} style={styles.legendItem}>
            <View
              style={[styles.legendSwatch, { backgroundColor: entry.color }]}
            />
            <Text style={styles.legendText}>{entry.label}</Text>
          </View>
        ))}
      </View>

      {markers.length > 0 ? (
        <View style={styles.markerRow}>
          {markers.map((marker) => (
            <Text
              key={marker.label}
              style={[styles.markerText, { color: marker.color }]}
            >
              {marker.label}
            </Text>
          ))}
        </View>
      ) : null}

      {xAxisLabel ? <Text style={styles.axisLabel}>{xAxisLabel}</Text> : null}
    </Card>
  );
};

const styles = StyleSheet.create({
  axisLabel: {
    color: colors.textMuted,
    fontSize: 11,
  },
  legendItem: {
    alignItems: 'center',
    flexDirection: 'row',
    gap: 6,
  },
  legendRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 12,
  },
  legendSwatch: {
    borderRadius: 2,
    height: 10,
    width: 10,
  },
  legendText: {
    color: colors.textSecondary,
    fontSize: 11,
  },
  markerRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 10,
  },
  markerText: {
    fontSize: 11,
    fontWeight: '600',
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

export default WaveformPlot;
