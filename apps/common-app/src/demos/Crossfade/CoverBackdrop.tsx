import React from 'react';
import { Image, ImageSourcePropType, StyleSheet, View } from 'react-native';
import Animated, {
  SharedValue,
  useAnimatedStyle,
} from 'react-native-reanimated';

interface CoverBackdropProps {
  covers: ImageSourcePropType[];
  /** Index of the shown cover; fractions blend two neighbours. */
  progress: SharedValue<number>;
}

const blurRadius = 40;
/** Blurring fades the image edges, so it's drawn larger than the screen. */
const bleed = 60;

interface BackdropLayerProps {
  cover: ImageSourcePropType;
  index: number;
  progress: SharedValue<number>;
}

/**
 * Layers are stacked in index order: the one at `floor(progress)` is opaque and
 * the next one fades in over it; the rest are hidden.
 */
const BackdropLayer: React.FC<BackdropLayerProps> = ({
  cover,
  index,
  progress,
}) => {
  const style = useAnimatedStyle(() => {
    const base = Math.floor(progress.value);
    if (index === base) {
      return { opacity: 1 };
    }
    return { opacity: index === base + 1 ? progress.value - base : 0 };
  });

  return (
    <Animated.View style={[StyleSheet.absoluteFill, style]}>
      <Image source={cover} blurRadius={blurRadius} style={styles.cover} />
    </Animated.View>
  );
};

/**
 * The current cover, blurred and dimmed, filling the screen behind the player.
 * It blends to the next cover along with the audio crossfade.
 */
const CoverBackdrop: React.FC<CoverBackdropProps> = ({ covers, progress }) => (
  <View style={styles.container} pointerEvents="none">
    {covers.map((cover, index) => (
      <BackdropLayer
        key={index}
        cover={cover}
        index={index}
        progress={progress}
      />
    ))}
    <View style={styles.dim} />
  </View>
);

export default CoverBackdrop;

const styles = StyleSheet.create({
  container: {
    ...StyleSheet.absoluteFill,
    overflow: 'hidden',
  },
  cover: {
    position: 'absolute',
    top: -bleed,
    left: -bleed,
    right: -bleed,
    bottom: -bleed,
    width: undefined,
    height: undefined,
  },
  dim: {
    ...StyleSheet.absoluteFill,
    experimental_backgroundImage:
      'linear-gradient(180deg, rgba(20, 18, 40, 0.35) 0%, rgba(20, 18, 40, 0.75) 60%, rgba(20, 18, 40, 0.9) 100%)',
  },
});
