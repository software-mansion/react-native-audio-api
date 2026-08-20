import { useEffect, useMemo } from 'react';
import {
  cancelAnimation,
  Easing,
  useSharedValue,
  useAnimatedStyle,
  withRepeat,
  withTiming,
} from 'react-native-reanimated';
import type { SharedValue } from 'react-native-reanimated';

export function formatTime(seconds: number): string {
  if (!Number.isFinite(seconds) || seconds < 0) {
    return '0:00';
  }
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = Math.floor(seconds % 60);
  if (h > 0) {
    return `${h}:${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
  } else {
    return `${m}:${s.toString().padStart(2, '0')}`;
  }
}

export function timeFromLocationX(
  locationX: number,
  trackWidth: number,
  durationSeconds: number
): number {
  if (trackWidth <= 0 || durationSeconds <= 0) {
    return 0;
  }
  const pct = Math.max(0, Math.min(1, locationX / trackWidth));
  return pct * durationSeconds;
}

export function useExpandableTrackHeight(
  trackBarHeight: number,
  trackBarHeightPressed: number,
  trackBarAnimMs: number
) {
  const height = useSharedValue(trackBarHeight);
  const animatedStyle = useAnimatedStyle(() => ({
    height: height.value,
    borderRadius: height.value / 2,
  }));

  return useMemo(
    () => ({
      animatedStyle,
      expand: () => {
        height.value = withTiming(trackBarHeightPressed, {
          duration: trackBarAnimMs,
        });
      },
      collapse: () => {
        height.value = withTiming(trackBarHeight, {
          duration: trackBarAnimMs,
        });
      },
    }),
    [
      animatedStyle,
      height,
      trackBarHeight,
      trackBarHeightPressed,
      trackBarAnimMs,
    ]
  );
}

/**
 * Indeterminate buffering sweep: a highlight that repeatedly travels the width
 * of the progress track while playback is stalled. It says "still waiting", not
 * "this much is buffered" — there is no buffered-ahead figure behind it.
 *
 * `trackWidth` is a shared value because the track is measured on layout, after
 * the first render.
 */
export function useBufferingSweep(
  isBuffering: boolean,
  trackWidth: SharedValue<number>,
  sweepWidthRatio: number,
  sweepDurationMs: number
) {
  const sweepProgress = useSharedValue(0);

  useEffect(() => {
    if (!isBuffering) {
      return;
    }

    sweepProgress.value = withRepeat(
      withTiming(1, {
        duration: sweepDurationMs,
        easing: Easing.inOut(Easing.quad),
      }),
      -1,
      false
    );

    return () => {
      cancelAnimation(sweepProgress);
      sweepProgress.value = 0;
    };
  }, [isBuffering, sweepDurationMs, sweepProgress]);

  return useAnimatedStyle(() => {
    const sweepWidth = trackWidth.value * sweepWidthRatio;
    // Travel the track plus the sweep's own width, starting one width off the
    // left edge, so it slides fully in and fully out instead of popping.
    const travelDistance = trackWidth.value + sweepWidth;

    return {
      width: sweepWidth,
      transform: [
        { translateX: -sweepWidth + sweepProgress.value * travelDistance },
      ],
    };
  });
}
