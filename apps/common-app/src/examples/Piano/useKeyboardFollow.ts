import { useMemo } from 'react';
import Animated, {
  AnimatedRef,
  FrameInfo,
  scrollTo,
  SharedValue,
  useFrameCallback,
  useSharedValue,
} from 'react-native-reanimated';

import { buildFollowTrack, followSampleInterval } from './followTrack';
import type { ScoreNote } from './usePianoDevApi';

/** Seconds the keyboard takes to cover about two thirds of a move. */
const glideTime = 0.3;

interface KeyboardFollowOptions {
  score: ScoreNote[];
  /** Score time in seconds, negative before the first note. */
  elapsed: SharedValue<number>;
  scrollRef: AnimatedRef<Animated.ScrollView>;
  scrollOffset: SharedValue<number>;
  viewportWidth: number;
}

/**
 * Slides the keyboard along with a playing score, so the keys carrying the
 * melody stay on screen (see `buildFollowTrack`). Runs on the UI thread and
 * eases toward each new position instead of jumping.
 */
export function useKeyboardFollow({
  score,
  elapsed,
  scrollRef,
  scrollOffset,
  viewportWidth,
}: KeyboardFollowOptions) {
  const track = useMemo(
    () => buildFollowTrack(score, viewportWidth),
    [score, viewportWidth]
  );
  const position = useSharedValue<number | null>(null);

  useFrameCallback((frame: FrameInfo) => {
    if (track.length === 0) {
      return;
    }

    const sample = Math.min(
      track.length - 1,
      Math.max(0, elapsed.value / followSampleInterval)
    );
    const before = Math.floor(sample);
    const after = Math.min(track.length - 1, before + 1);
    const target =
      track[before] + (track[after] - track[before]) * (sample - before);

    const current = position.value ?? scrollOffset.value;
    const frameSeconds = (frame.timeSincePreviousFrame ?? 16) / 1000;
    const next =
      current + (target - current) * (1 - Math.exp(-frameSeconds / glideTime));
    position.value = next;

    if (Math.abs(next - scrollOffset.value) > 0.5) {
      scrollTo(scrollRef, next, 0, false);
    }
  });
}
