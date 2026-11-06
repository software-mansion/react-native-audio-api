import React from 'react';
import { ImageSourcePropType, StyleSheet, View } from 'react-native';
import Animated, {
  SharedValue,
  useAnimatedStyle,
} from 'react-native-reanimated';

interface ArtworkStackProps {
  covers: ImageSourcePropType[];
  /** Index of the shown cover; fractions are mid-swipe. */
  progress: SharedValue<number>;
  size: number;
}

/** How far the upcoming cover peeks out from behind the current one. */
const peekOffset = 30;
const peekScale = 0.9;
const peekOpacity = 0.55;
const gapWhenSwipedAway = 24;

interface ArtworkCoverProps {
  cover: ImageSourcePropType;
  index: number;
  progress: SharedValue<number>;
  size: number;
}

const ArtworkCover: React.FC<ArtworkCoverProps> = ({
  cover,
  index,
  progress,
  size,
}) => {
  const style = useAnimatedStyle(() => {
    const distance = index - progress.value;
    if (distance <= 0) {
      // The previous cover stays one slide to the left, its edge just visible.
      return {
        opacity: distance >= -1 ? 1 : 0,
        transform: [
          { translateX: distance * (size + gapWhenSwipedAway) },
          { scale: 1 },
        ],
      };
    }
    // Only the next cover peeks; later ones wait, hidden, behind it.
    const peek = Math.min(distance, 1);
    return {
      opacity: distance <= 1 ? 1 - (1 - peekOpacity) * peek : 0,
      transform: [
        { translateX: peek * peekOffset },
        { scale: 1 - (1 - peekScale) * peek },
      ],
    };
  });

  return (
    <Animated.Image
      source={cover}
      style={[styles.cover, { width: size, height: size }, style]}
    />
  );
};

/**
 * The covers, positioned by their distance from `progress`: the current one
 * centred, the next one tucked behind it to the right, and the previous one
 * slid off to the left. Swiping moves `progress`, so the next cover grows out
 * of its peek while the current one slides away. The swipe itself is handled by
 * the screen, over a wider area than the artwork.
 */
const ArtworkStack: React.FC<ArtworkStackProps> = ({
  covers,
  progress,
  size,
}) => (
  <View
    style={[styles.stack, { width: size, height: size }]}
    accessibilityLabel="Album artwork, swipe to crossfade"
  >
    {/* Later covers are drawn first, so earlier ones stay on top. */}
    {covers
      .map((cover, index) => (
        <ArtworkCover
          key={index}
          cover={cover}
          index={index}
          progress={progress}
          size={size}
        />
      ))
      .reverse()}
  </View>
);

export default ArtworkStack;

const styles = StyleSheet.create({
  stack: {
    alignSelf: 'center',
  },
  cover: {
    position: 'absolute',
    borderRadius: 22,
    boxShadow: '0 20px 40px rgba(0, 0, 0, 0.45)',
  },
});
