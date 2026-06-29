import { useMemo } from 'react';
import {
  useSharedValue,
  useAnimatedStyle,
  withTiming,
} from 'react-native-reanimated';

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
