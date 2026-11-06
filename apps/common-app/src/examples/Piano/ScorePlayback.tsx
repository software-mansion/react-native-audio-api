import React from 'react';
import Animated, {
  AnimatedRef,
  FrameInfo,
  SharedValue,
  useFrameCallback,
  useSharedValue,
} from 'react-native-reanimated';

import FallingNotes from './FallingNotes';
import { scoreStartDelay, type ScoreNote } from './usePianoDevApi';
import { useKeyboardFollow } from './useKeyboardFollow';

/**
 * Score time in seconds on the UI thread, counted from mount. The score's first
 * note sounds `scoreStartDelay` after `play()`, so the clock starts that far
 * below zero.
 */
function useScoreClock() {
  const startedAt = useSharedValue(-1);
  const elapsed = useSharedValue(-scoreStartDelay);

  useFrameCallback((frame: FrameInfo) => {
    if (startedAt.value < 0) {
      startedAt.value = frame.timestamp + scoreStartDelay * 1000;
    }
    elapsed.value = (frame.timestamp - startedAt.value) / 1000;
  });

  return elapsed;
}

interface ScorePlaybackProps {
  score: ScoreNote[];
  scrollRef: AnimatedRef<Animated.ScrollView>;
  scrollOffset: SharedValue<number>;
  viewportWidth: number;
}

/**
 * Shown while the app plays a score on its own: notes fall onto the keys and
 * the keyboard slides to keep the melody on screen.
 */
const ScorePlayback: React.FC<ScorePlaybackProps> = ({
  score,
  scrollRef,
  scrollOffset,
  viewportWidth,
}) => {
  const elapsed = useScoreClock();
  useKeyboardFollow({ score, elapsed, scrollRef, scrollOffset, viewportWidth });

  return (
    <FallingNotes score={score} elapsed={elapsed} scrollOffset={scrollOffset} />
  );
};

export default ScorePlayback;
