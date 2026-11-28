import React, { memo, useCallback, useEffect } from 'react';
import { Pressable, StyleSheet, useWindowDimensions, View } from 'react-native';
import { GestureDetector, usePanGesture } from 'react-native-gesture-handler';
import Animated, {
  SharedValue,
  useAnimatedStyle,
  useSharedValue,
  withSpring,
} from 'react-native-reanimated';
import { scheduleOnRN, scheduleOnUI } from 'react-native-worklets';

import { colors } from '../../styles';
import type { Track } from '../Crossfade/tracks';

const maxCoverSize = 280;

/** Side of the current cover on a screen `screenWidth` wide. */
export function queueCoverSize(screenWidth: number) {
  return Math.min(screenWidth * 0.56, maxCoverSize);
}

/** Corner radius of a cover as seen on screen, mini player included. */
const coverCornerRadius = 8;
const backShadowOpacity = 0.25;
const frontShadowOpacity = 0.4;
/**
 * Sheet expansion from which the neighbours fade in: until then the current
 * cover travels alone from the mini player.
 */
const neighbourRevealStart = 0.6;

const clamp01 = (value: number) => {
  'worklet';
  return Math.min(1, Math.max(0, value));
};

/** The swipe starts after this much horizontal travel (pt). */
const swipeActivation = 10;
/** Vertical travel (pt) that hands the touch to the sheet instead. */
const swipeVerticalTolerance = 24;
/** Share of a step a swipe has to travel to move on a track. */
const commitShare = 0.3;
/** A flick this fast (pt/s) moves on a track even if the swipe was short. */
const flickVelocity = 450;
/** How much of the drag past one track (or the queue's ends) still moves it. */
const rubberBandFactor = 0.3;
const settleStiffness = 220;
/** Natural frequency (rad/s) of the settle spring, at the default mass of 1. */
const settleFrequency = Math.sqrt(settleStiffness);
/**
 * Critically damped (damping = 2√stiffness), so the covers settle without
 * wobbling.
 */
const settleSpring = {
  stiffness: settleStiffness,
  damping: 2 * settleFrequency,
};
/**
 * Fastest start of a glide, in tracks per second. A critically damped spring
 * that starts at v passes its target by at most v / (e × frequency); this keeps
 * a hard flick's overshoot under a fifth of a track.
 */
const maxGlideSpeed = 0.2 * Math.E * settleFrequency;

/** `value` clamped to `min..max`, with the excess eased instead of cut off. */
function rubberBand(value: number, min: number, max: number) {
  'worklet';
  if (value < min) {
    return min - (min - value) * rubberBandFactor;
  }
  if (value > max) {
    return max + (value - max) * rubberBandFactor;
  }
  return value;
}

/** Scale and opacity of the previous and next covers, beside the current one. */
const neighbourScale = 0.84;
const neighbourOpacity = 0.6;
/** Share of a neighbour's width tucked behind the current cover. */
const neighbourTuck = 0.2;

/**
 * Distance between the centres of neighbouring covers: the neighbours sit
 * slightly behind the current cover and run off the screen's edges.
 */
function coverSpacing(size: number) {
  return size / 2 + neighbourScale * size * (0.5 - neighbourTuck);
}

interface QueueCarouselProps {
  tracks: Track[];
  currentIndex: number;
  /** Fractional index of the centred cover, shared with the backdrop. */
  position: SharedValue<number>;
  /**
   * The player sheet's expansion, 0 (mini) to 1 (full). The sheet scales the
   * whole queue down into the mini player; here the neighbours fade and the
   * corners keep their on-screen radius through the scale.
   */
  expansion: SharedValue<number>;
  /** Side of the current cover in the mini player, where the queue starts. */
  collapsedCoverSize: number;
  onSelect: (index: number) => void;
}

interface QueueCoverProps {
  track: Track;
  index: number;
  position: SharedValue<number>;
  expansion: SharedValue<number>;
  size: number;
  /** The sheet's scale of the queue at the mini player. */
  collapsedScale: number;
  spacing: number;
  /**
   * `front` draws only while this cover is the nearest to the centre, over
   * every `back` cover; see `QueueCarousel`.
   */
  layer: 'back' | 'front';
  onPress: (index: number) => void;
}

/** Takes no per-track props, so changing track doesn't re-render the covers. */
const QueueCover = memo<QueueCoverProps>(function QueueCover({
  track,
  index,
  position,
  expansion,
  size,
  collapsedScale,
  spacing,
  layer,
  onPress,
}) {
  const isFront = layer === 'front';
  const style = useAnimatedStyle(() => {
    const distance = index - position.value;
    // Covers further than one step are off screen; they keep the neighbour look.
    const steps = Math.min(Math.abs(distance), 1);
    const neighbourLook = 1 - steps * (1 - neighbourOpacity);
    const isCurrent = Math.abs(distance) < 0.5;
    const layerOpacity = isFront ? (isCurrent ? 1 : 0) : neighbourLook;
    const reveal = clamp01(
      (expansion.value - neighbourRevealStart) / (1 - neighbourRevealStart)
    );
    return {
      opacity: isCurrent ? layerOpacity : layerOpacity * reveal,
      transform: [
        { translateX: distance * spacing },
        { scale: 1 - steps * (1 - neighbourScale) },
      ],
    };
  });

  // The radius is laid out before the sheet's scale, so it is divided by
  // that scale to stay the same on screen as the cover grows.
  const cornerRadius = () => {
    'worklet';
    const sheetScale = collapsedScale + (1 - collapsedScale) * expansion.value;
    return coverCornerRadius / sheetScale;
  };
  const frameCorners = useAnimatedStyle(() => ({
    borderRadius: cornerRadius(),
  }));
  const imageCorners = useAnimatedStyle(() => ({
    borderRadius: cornerRadius(),
  }));

  const image = (
    <Animated.Image
      source={track.cover}
      style={[{ width: size, height: size }, imageCorners]}
    />
  );

  return (
    <Animated.View
      style={[
        styles.cover,
        isFront ? styles.frontShadow : styles.backShadow,
        { width: size, height: size },
        style,
        frameCorners,
      ]}
    >
      {isFront ? (
        image
      ) : (
        <Pressable
          onPress={() => onPress(index)}
          accessibilityRole="button"
          accessibilityLabel={`Play ${track.title}`}
        >
          {image}
        </Pressable>
      )}
    </Animated.View>
  );
});

/**
 * The play queue around the current track: its cover in the middle, with the
 * previous and next ones slightly behind it, peeking in at the sides.
 *
 * Overlapping covers need a stacking order, and an animated `zIndex` goes
 * through a shadow-tree commit that lags behind transforms and opacity (a fast
 * swipe briefly showed the wrong cover on top). So every cover is drawn twice:
 * once in the back layer, and once in a front layer where only the cover
 * nearest the centre is opaque. Opacity updates on the UI thread in the same
 * frame as the transforms, so the nearest cover is always on top.
 *
 * Swiping moves one track either way and tapping a neighbour jumps to it; the
 * track starts playing at once while the queue glides to centre it.
 */
const QueueCarousel = memo<QueueCarouselProps>(function QueueCarousel({
  tracks,
  currentIndex,
  position,
  expansion,
  collapsedCoverSize,
  onSelect,
}) {
  const { width } = useWindowDimensions();
  const size = queueCoverSize(width);
  const collapsedScale = collapsedCoverSize / size;
  // A swipe of one cover spacing moves the queue by one track, so the covers
  // stay under the finger.
  const stepWidth = coverSpacing(size);
  const lastIndex = tracks.length - 1;
  /** The track the queue is gliding to, ahead of the screen's selection. */
  const target = useSharedValue(currentIndex);
  /** The screen's current track, as last reported. */
  const selected = useSharedValue(currentIndex);
  const dragOrigin = useSharedValue(currentIndex);

  /** Selects `index` and glides there from `velocity` (tracks/s). */
  const glideTo = useCallback(
    (index: number, velocity: number) => {
      'worklet';
      const startVelocity = Math.min(
        maxGlideSpeed,
        Math.max(-maxGlideSpeed, velocity)
      );
      target.value = index;
      position.value = withSpring(index, {
        ...settleSpring,
        velocity: startVelocity,
      });
      // Plays right away; the screen's re-render can't disturb the glide, as
      // the covers take no per-track props and stack by opacity.
      if (index !== selected.value) {
        scheduleOnRN(onSelect, index);
      }
    },
    [onSelect, position, selected, target]
  );

  const onCoverPress = useCallback(
    (index: number) => scheduleOnUI(glideTo, index, 0),
    [glideTo]
  );

  // The transport buttons, the playlist and the end of a track change the
  // track from outside. A glide that ended in this selection is already there.
  useEffect(() => {
    selected.value = currentIndex;
    if (target.value !== currentIndex) {
      target.value = currentIndex;
      position.value = withSpring(currentIndex, settleSpring);
    }
  }, [currentIndex, position, selected, target]);

  const pan = usePanGesture({
    activeOffsetX: [-swipeActivation, swipeActivation],
    failOffsetY: [-swipeVerticalTolerance, swipeVerticalTolerance],
    onActivate: () => {
      // Picks the queue up where it is, even mid-glide, without a jump.
      dragOrigin.value = position.value;
    },
    onUpdate: (event) => {
      const start = target.value;
      position.value = rubberBand(
        dragOrigin.value - event.translationX / stepWidth,
        Math.max(0, start - 1),
        Math.min(lastIndex, start + 1)
      );
    },
    onDeactivate: (event) => {
      const start = target.value;
      const travelled = position.value - start;
      // Swiping left (negative velocity) moves forward through the queue.
      const flickDirection = -Math.sign(event.velocityX);
      let next = start;
      if (Math.abs(event.velocityX) > flickVelocity) {
        next = start + flickDirection;
      } else if (Math.abs(travelled) > commitShare) {
        next = start + Math.sign(travelled);
      }
      next = Math.min(lastIndex, Math.max(0, next));
      // The glide starts at the finger's speed, in tracks per second.
      glideTo(next, -event.velocityX / stepWidth);
    },
  });

  return (
    <GestureDetector gesture={pan}>
      <View style={[styles.row, { height: size }]}>
        {tracks.map((track, index) => (
          <QueueCover
            key={index}
            track={track}
            index={index}
            position={position}
            expansion={expansion}
            size={size}
            collapsedScale={collapsedScale}
            spacing={stepWidth}
            layer="back"
            onPress={onCoverPress}
          />
        ))}
        {/* Touches pass through to the back layer's covers. */}
        <View
          style={styles.frontLayer}
          pointerEvents="none"
          accessibilityElementsHidden
          importantForAccessibility="no-hide-descendants"
        >
          {tracks.map((track, index) => (
            <QueueCover
              key={index}
              track={track}
              index={index}
              position={position}
              expansion={expansion}
              size={size}
              collapsedScale={collapsedScale}
              spacing={stepWidth}
              layer="front"
              onPress={onCoverPress}
            />
          ))}
        </View>
      </View>
    </GestureDetector>
  );
});

export default QueueCarousel;

const styles = StyleSheet.create({
  row: {
    alignItems: 'center',
    justifyContent: 'center',
  },
  cover: {
    position: 'absolute',
    // An opaque fill lets iOS take the shadow's shape from the bounds instead
    // of rendering it offscreen every frame.
    backgroundColor: colors.recess,
    shadowColor: '#000',
    shadowRadius: 16,
    shadowOffset: { width: 0, height: 8 },
  },
  backShadow: {
    shadowOpacity: backShadowOpacity,
  },
  // Falls on the neighbours, so they read as behind the current cover.
  frontShadow: {
    shadowOpacity: frontShadowOpacity,
  },
  frontLayer: {
    ...StyleSheet.absoluteFill,
    alignItems: 'center',
    justifyContent: 'center',
  },
});
