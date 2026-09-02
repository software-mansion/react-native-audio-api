import { useEffect, useMemo, useRef } from 'react';
import { Animated, Easing } from 'react-native';

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
  const height = useRef(new Animated.Value(trackBarHeight)).current;

  return useMemo(() => {
    const animateTo = (toValue: number) => {
      Animated.timing(height, {
        toValue,
        duration: trackBarAnimMs,
        useNativeDriver: false,
      }).start();
    };

    return {
      animatedStyle: {
        height,
        borderRadius: Animated.divide(height, 2),
      },
      expand: () => animateTo(trackBarHeightPressed),
      collapse: () => animateTo(trackBarHeight),
    };
  }, [height, trackBarHeight, trackBarHeightPressed, trackBarAnimMs]);
}

/**
 * Indeterminate buffering sweep: a highlight that repeatedly travels the width
 * of the progress track while playback is stalled. It says "still waiting", not
 * "this much is buffered" — there is no buffered-ahead figure behind it.
 *
 * `trackWidth` is measured on layout, so it is zero until after the first
 * render and the sweep simply has nothing to travel until then.
 */
export function useBufferingSweep(
  isBuffering: boolean,
  trackWidth: number,
  sweepWidthRatio: number,
  sweepDurationMs: number
) {
  const sweepProgress = useRef(new Animated.Value(0)).current;

  useEffect(() => {
    if (!isBuffering) {
      return;
    }

    const sweep = Animated.loop(
      Animated.timing(sweepProgress, {
        toValue: 1,
        duration: sweepDurationMs,
        easing: Easing.inOut((t) => Easing.quad(t)),
        useNativeDriver: true,
      })
    );
    sweep.start();

    return () => {
      sweep.stop();
      sweepProgress.setValue(0);
    };
  }, [isBuffering, sweepDurationMs, sweepProgress]);

  const sweepWidth = trackWidth * sweepWidthRatio;

  return {
    width: sweepWidth,
    transform: [
      {
        translateX: sweepProgress.interpolate({
          inputRange: [0, 1],
          outputRange: [-sweepWidth, trackWidth],
        }),
      },
    ],
  };
}
