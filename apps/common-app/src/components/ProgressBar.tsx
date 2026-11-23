import React, { FC } from 'react';
import { StyleProp, StyleSheet, View, ViewStyle } from 'react-native';

import { colors } from '../styles';

interface ProgressBarProps {
  progress: number;
  color?: string;
  height?: number;
  style?: StyleProp<ViewStyle>;
}

/** A thin track with a filled share, for playback position and levels. */
const ProgressBar: FC<ProgressBarProps> = ({
  progress,
  color = colors.accent,
  height = 4,
  style,
}) => (
  <View
    style={[styles.track, { height, borderRadius: height / 2 }, style]}
    accessibilityRole="progressbar"
    accessibilityValue={{ min: 0, max: 100, now: Math.round(progress * 100) }}
  >
    <View
      style={[
        styles.fill,
        {
          width: `${Math.min(1, Math.max(0, progress)) * 100}%`,
          backgroundColor: color,
        },
      ]}
    />
  </View>
);

export default ProgressBar;

const styles = StyleSheet.create({
  track: {
    overflow: 'hidden',
    backgroundColor: colors.track,
  },
  fill: {
    height: '100%',
  },
});
