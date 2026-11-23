import React, { FC, useCallback, useRef, useState } from 'react';
import {
  AccessibilityActionEvent,
  LayoutChangeEvent,
  StyleProp,
  StyleSheet,
  View,
  ViewStyle,
} from 'react-native';
import { GestureDetector, usePanGesture } from 'react-native-gesture-handler';
import { useSharedValue } from 'react-native-reanimated';
import { scheduleOnRN } from 'react-native-worklets';

import { colors } from '../styles';

interface ProgressBarProps {
  /** 0..1; values outside are clamped. */
  progress: number;
  color?: string;
  height?: number;
  style?: StyleProp<ViewStyle>;
  /**
   * Makes the bar seekable: drag or tap along it, and this is called with the
   * chosen progress (0..1) when the finger lifts.
   */
  onSeek?: (progress: number) => void;
  /** While seeking, the progress under the finger; `null` once it lifts. */
  onScrub?: (progress: number | null) => void;
}

const thumbSize = 12;
/** Extra touch room above and below a seekable bar (pt). */
const touchMargin = 12;
/** How far an accessibility increment or decrement moves the progress. */
const accessibilityStep = 0.05;

const clamp01 = (value: number) => {
  'worklet';
  return Math.min(1, Math.max(0, value));
};

const accessibilityActions = [
  { name: 'increment' },
  { name: 'decrement' },
] as const;

/**
 * A thin track with a filled share, for playback position and levels. With
 * `onSeek` it gains a thumb and follows a drag or tap along it.
 */
const ProgressBar: FC<ProgressBarProps> = ({
  progress,
  color = colors.accent,
  height = 4,
  style,
  onSeek,
  onScrub,
}) => {
  // Read inside the gesture's worklets, so a shared value rather than state.
  const width = useSharedValue(0);
  /** The progress under the finger while seeking, shown instead of `progress`. */
  const [scrubbed, setScrubbed] = useState<number | null>(null);
  const lastScrubbed = useSharedValue(0);

  // The gesture's callbacks are created once; these read the latest props.
  const callbacks = useRef({ onSeek, onScrub });
  callbacks.current = { onSeek, onScrub };

  const scrubTo = useCallback((value: number) => {
    setScrubbed(value);
    callbacks.current.onScrub?.(value);
  }, []);

  const release = useCallback((value: number) => {
    setScrubbed(null);
    callbacks.current.onScrub?.(null);
    callbacks.current.onSeek?.(value);
  }, []);

  // `minDistance: 0` activates on touch-down, so one gesture covers both a tap
  // and a drag.
  const pan = usePanGesture({
    minDistance: 0,
    onBegin: (event) => {
      lastScrubbed.value = clamp01(event.x / width.value);
      scheduleOnRN(scrubTo, lastScrubbed.value);
    },
    onUpdate: (event) => {
      lastScrubbed.value = clamp01(event.x / width.value);
      scheduleOnRN(scrubTo, lastScrubbed.value);
    },
    onFinalize: () => {
      scheduleOnRN(release, lastScrubbed.value);
    },
  });

  const shown = clamp01(scrubbed ?? progress);
  const percent = `${shown * 100}%` as const;

  const track = (
    <View
      style={[
        styles.track,
        { height, borderRadius: height / 2 },
        !onSeek && style,
      ]}
      accessibilityRole={onSeek ? undefined : 'progressbar'}
      accessibilityValue={{ min: 0, max: 100, now: Math.round(shown * 100) }}
    >
      <View style={[styles.fill, { width: percent, backgroundColor: color }]} />
    </View>
  );

  if (!onSeek) {
    return track;
  }

  const onLayout = (event: LayoutChangeEvent) => {
    width.value = event.nativeEvent.layout.width;
  };

  const onAccessibilityAction = (event: AccessibilityActionEvent) => {
    const step =
      event.nativeEvent.actionName === 'increment'
        ? accessibilityStep
        : -accessibilityStep;
    onSeek(clamp01(progress + step));
  };

  return (
    <GestureDetector gesture={pan}>
      <View
        style={[styles.touchArea, style]}
        onLayout={onLayout}
        accessible
        accessibilityRole="adjustable"
        accessibilityValue={{ min: 0, max: 100, now: Math.round(shown * 100) }}
        accessibilityActions={accessibilityActions}
        onAccessibilityAction={onAccessibilityAction}
      >
        {track}
        <View
          pointerEvents="none"
          style={[
            styles.thumb,
            {
              left: percent,
              backgroundColor: color,
              // Grows while held, like the system's playback scrubbers.
              transform: [{ scale: scrubbed === null ? 1 : 1.4 }],
            },
          ]}
        />
      </View>
    </GestureDetector>
  );
};

export default ProgressBar;

const styles = StyleSheet.create({
  track: {
    overflow: 'hidden',
    backgroundColor: colors.track,
  },
  fill: {
    height: '100%',
  },
  touchArea: {
    paddingVertical: touchMargin,
    justifyContent: 'center',
  },
  thumb: {
    position: 'absolute',
    width: thumbSize,
    height: thumbSize,
    marginLeft: -thumbSize / 2,
    borderRadius: thumbSize / 2,
  },
});
