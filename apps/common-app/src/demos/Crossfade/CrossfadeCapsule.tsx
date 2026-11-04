import {
  isLiquidGlassSupported,
  LiquidGlassView,
} from '@callstack/liquid-glass';
import React, { useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import Animated, {
  SharedValue,
  useAnimatedReaction,
  useAnimatedStyle,
  withTiming,
} from 'react-native-reanimated';
import { scheduleOnRN } from 'react-native-worklets';

import { AppIcon } from '../../components';
import { colors } from '../../styles';

interface CrossfadeCapsuleProps {
  /** Index of the current track; fractions are mid-crossfade. */
  progress: SharedValue<number>;
  /** Track index the crossfade is heading to. */
  destination: SharedValue<number>;
  destinationTitle: string;
}

const height = 34;
const barWidth = 46;

/**
 * "Crossfading to …" with a bar filling up as the transition runs. It shows
 * while the mix is between two tracks, from a swipe or a skip button, and keeps
 * its space while hidden so the player doesn't shift.
 *
 * Animating opacity on a glass view or its ancestors stops the material from
 * rendering, so the glass is switched with its native `effect` instead and only
 * the contents fade.
 */
const CrossfadeCapsule: React.FC<CrossfadeCapsuleProps> = ({
  progress,
  destination,
  destinationTitle,
}) => {
  const [isVisible, setIsVisible] = useState(false);

  useAnimatedReaction(
    () => Math.abs(progress.value - Math.round(progress.value)) > 0.002,
    (isCrossfading: boolean, wasCrossfading: boolean | null) => {
      if (isCrossfading !== wasCrossfading) {
        scheduleOnRN(setIsVisible, isCrossfading);
      }
    }
  );

  const contentStyle = useAnimatedStyle(() => ({
    opacity: withTiming(isVisible ? 1 : 0, { duration: 180 }),
  }));

  const fillStyle = useAnimatedStyle(() => {
    const travelled = 1 - Math.abs(destination.value - progress.value);
    return { width: Math.min(1, Math.max(0, travelled)) * barWidth };
  });

  return (
    <View
      style={styles.slot}
      accessibilityElementsHidden={!isVisible}
      importantForAccessibility={isVisible ? 'auto' : 'no-hide-descendants'}
      accessibilityLiveRegion="polite"
      accessibilityLabel={`Crossfading to ${destinationTitle}`}
    >
      <LiquidGlassView
        colorScheme="dark"
        effect={isVisible ? 'regular' : 'none'}
        style={[
          styles.capsule,
          !isLiquidGlassSupported && isVisible && styles.fallback,
        ]}
      >
        <Animated.View style={[styles.content, contentStyle]}>
          <AppIcon
            name="arrow-left-right"
            size={14}
            color={colors.textPrimary}
          />
          <Text style={styles.label} numberOfLines={1}>
            Crossfading to “{destinationTitle}”
          </Text>
          <View style={styles.bar}>
            <Animated.View style={[styles.fill, fillStyle]} />
          </View>
        </Animated.View>
      </LiquidGlassView>
    </View>
  );
};

export default CrossfadeCapsule;

const styles = StyleSheet.create({
  slot: {
    height,
    alignItems: 'center',
  },
  capsule: {
    height,
    borderRadius: height / 2,
    paddingHorizontal: 12,
    justifyContent: 'center',
  },
  content: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  fallback: {
    backgroundColor: 'rgba(255, 255, 255, 0.12)',
  },
  label: {
    fontSize: 12.5,
    fontWeight: '600',
    color: colors.textPrimary,
  },
  bar: {
    width: barWidth,
    height: 4,
    borderRadius: 2,
    overflow: 'hidden',
    backgroundColor: 'rgba(255, 255, 255, 0.2)',
  },
  fill: {
    height: '100%',
    backgroundColor: colors.white,
  },
});
