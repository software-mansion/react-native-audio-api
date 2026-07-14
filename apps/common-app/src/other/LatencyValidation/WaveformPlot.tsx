import React, { FC, useMemo } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import Svg, { Line, Path, Rect } from 'react-native-svg';

import { colors } from '../../styles';
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

  const maxVal = Math.max(scaleMax, 0.001);

  return points
    .map((point, index) => {
      const x =
        points.length <= 1 ? 0 : (index / (points.length - 1)) * width;
      const y = height - (Math.abs(point) / maxVal) * (height * 0.82) - 6;
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
  const scaleMax = useMemo(() => {
    if (!sharedScale) {
      return Math.max(
        ...series.flatMap((entry) => entry.points.map((point) => Math.abs(point))),
        0.001
      );
    }

    return Math.max(
      ...series.flatMap((entry) => entry.points.map((point) => Math.abs(point))),
      0.001
    );
  }, [series, sharedScale]);

  const paths = useMemo(
    () =>
      series.map((entry) => {
        const entryMax = sharedScale
          ? scaleMax
          : Math.max(...entry.points.map((point) => Math.abs(point)), 0.001);

        return {
          ...entry,
          path: buildEnvelopePath(entry.points, width, height, entryMax),
        };
      }),
    [height, scaleMax, series, sharedScale, width]
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>{title}</Text>
      {caption ? <Text style={styles.caption}>{caption}</Text> : null}

      <View style={[styles.plotFrame, { width, height }]}>
        <Svg width={width} height={height}>
          <Rect
            x={0}
            y={0}
            width={width}
            height={height}
            fill={colors.backgroundDark}
            rx={4}
          />
          <Line
            x1={0}
            y1={height - 6}
            x2={width}
            y2={height - 6}
            stroke={colors.separator}
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
            <View style={[styles.legendSwatch, { backgroundColor: entry.color }]} />
            <Text style={styles.legendText}>{entry.label}</Text>
          </View>
        ))}
      </View>

      {markers.length > 0 ? (
        <View style={styles.markerRow}>
          {markers.map((marker) => (
            <Text key={marker.label} style={[styles.markerText, { color: marker.color }]}>
              {marker.label}
            </Text>
          ))}
        </View>
      ) : null}

      {xAxisLabel ? <Text style={styles.axisLabel}>{xAxisLabel}</Text> : null}
    </View>
  );
};

const styles = StyleSheet.create({
  axisLabel: {
    color: colors.gray,
    fontSize: 11,
    marginTop: 4,
  },
  caption: {
    color: colors.gray,
    fontSize: 12,
    lineHeight: 17,
    marginBottom: 8,
  },
  container: {
    backgroundColor: colors.backgroundLight,
    borderRadius: 8,
    gap: 8,
    padding: 12,
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
    color: colors.gray,
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
    borderColor: colors.separator,
    borderRadius: 4,
    borderWidth: StyleSheet.hairlineWidth,
    overflow: 'hidden',
  },
  title: {
    color: colors.white,
    fontSize: 15,
    fontWeight: '700',
  },
});

export default WaveformPlot;
