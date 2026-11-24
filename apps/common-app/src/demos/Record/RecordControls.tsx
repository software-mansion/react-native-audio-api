import React from 'react';
import { Pressable, StyleSheet, View } from 'react-native';
import Animated, {
  useAnimatedStyle,
  withTiming,
} from 'react-native-reanimated';

import { RoundButton } from '../../components';
import { colors } from '../../styles';
import { RecordingState } from './types';

const sideButtonSize = 52;
const recordRingSize = 72;
const recordDotSize = 56;
const stopSquareSize = 28;

interface RecordControlsProps {
  state: RecordingState;
  onRecordPress: () => void;
  onPausePress: () => void;
}

/**
 * Voice Memos style controls: the centre button records, and while a take is
 * running its red circle morphs into a stop square that ends and saves it.
 * Pause / Resume sits on the left; the right is left empty so the centre button
 * stays centred.
 */
const RecordControls: React.FC<RecordControlsProps> = ({
  state,
  onRecordPress,
  onPausePress,
}) => {
  const isTakeRunning =
    state === RecordingState.Recording || state === RecordingState.Paused;
  const isBusy =
    state === RecordingState.Starting || state === RecordingState.Saving;

  const dotStyle = useAnimatedStyle(() => {
    const size = isTakeRunning ? stopSquareSize : recordDotSize;
    return {
      width: withTiming(size),
      height: withTiming(size),
      borderRadius: withTiming(isTakeRunning ? 7 : recordDotSize / 2),
      opacity: withTiming(isBusy ? 0.4 : 1),
    };
  });

  return (
    <View style={styles.row}>
      <View style={styles.sideSlot}>
        {isTakeRunning && (
          <RoundButton
            variant="glass"
            icon={state === RecordingState.Paused ? 'play' : 'pause'}
            accessibilityLabel={
              state === RecordingState.Paused ? 'Resume' : 'Pause'
            }
            onPress={onPausePress}
            size={sideButtonSize}
            iconSize={22}
          />
        )}
      </View>

      <Pressable
        onPress={onRecordPress}
        disabled={isBusy}
        accessibilityRole="button"
        accessibilityLabel={isTakeRunning ? 'Stop and save' : 'Record'}
        accessibilityState={{ disabled: isBusy }}
        style={({ pressed }) => [styles.recordRing, pressed && styles.pressed]}
      >
        <Animated.View style={[styles.recordDot, dotStyle]} />
      </Pressable>

      <View style={styles.sideSlot} />
    </View>
  );
};

export default RecordControls;

const styles = StyleSheet.create({
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 26,
  },
  sideSlot: {
    width: sideButtonSize,
    height: sideButtonSize,
  },
  recordRing: {
    width: recordRingSize,
    height: recordRingSize,
    borderRadius: recordRingSize / 2,
    borderWidth: 4,
    borderColor: colors.textPrimary,
    alignItems: 'center',
    justifyContent: 'center',
  },
  pressed: {
    transform: [{ scale: 0.95 }],
  },
  recordDot: {
    backgroundColor: colors.accent,
  },
});
