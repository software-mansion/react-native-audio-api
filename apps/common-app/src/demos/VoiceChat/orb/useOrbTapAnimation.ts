import { useCallback } from 'react';
import {
  ReduceMotion,
  useAnimatedStyle,
  useSharedValue,
  withSequence,
  withTiming,
} from 'react-native-reanimated';

/** A short scale bump on tap; it transforms the orb, never resizes its canvas. */
export function useOrbTapAnimation() {
  const scale = useSharedValue(1);
  const animatedStyle = useAnimatedStyle(() => ({
    transform: [{ scale: scale.get() }],
  }));
  const pulse = useCallback(() => {
    scale.set(
      withSequence(
        ReduceMotion.System,
        withTiming(1.06, { duration: 130 }),
        withTiming(1, { duration: 260 })
      )
    );
  }, [scale]);

  return { animatedStyle, pulse };
}
