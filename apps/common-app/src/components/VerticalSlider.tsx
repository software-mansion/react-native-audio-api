import React, { useEffect } from 'react';
import { StyleSheet, Text, TextInput, View } from 'react-native';
import { GestureDetector, usePanGesture } from 'react-native-gesture-handler';
import Animated, {
  useAnimatedProps,
  useAnimatedStyle,
  useSharedValue,
  withSpring,
} from 'react-native-reanimated';
import { scheduleOnRN } from 'react-native-worklets';

const SLIDER_HEIGHT = 100;
const THUMB_SIZE = 30;
const TRACK_HEIGHT = SLIDER_HEIGHT - THUMB_SIZE;

const AnimatedValueText = Animated.createAnimatedComponent(TextInput);

interface VerticalSliderProps {
  label: string;
  /** Normalized position, 0 (bottom) to 1 (top). */
  value: number;
  labelColor?: string;
  valueColor?: string;
  onValueChange: (value: number) => void;
  /**
   * Snap points; when set, the fader snaps to the nearest one on release, draws
   * a tick for each and hides its numeric value.
   */
  possibleValues?: number[];
}

/**
 * A hardware-style fader for the pedal bodies: a thin slot with a square cap
 * you drag up and down, the label and value printed on the pedal.
 */
const VerticalSlider: React.FC<VerticalSliderProps> = ({
  label,
  value,
  labelColor = '#333',
  valueColor = '#555',
  onValueChange,
  possibleValues,
}) => {
  const progress = useSharedValue(value);
  const startValue = useSharedValue(0);
  const hasSnapPoints = !!possibleValues && possibleValues.length > 0;

  useEffect(() => {
    progress.value = withSpring(value);
  }, [value, progress]);

  const findClosestValue = (currentValue: number): number => {
    'worklet';
    if (!possibleValues || possibleValues.length === 0) {
      return currentValue;
    }

    let closest = possibleValues[0];
    let minDistance = Math.abs(currentValue - closest);

    for (const possibleValue of possibleValues) {
      const distance = Math.abs(currentValue - possibleValue);
      if (distance < minDistance) {
        minDistance = distance;
        closest = possibleValue;
      }
    }

    return closest;
  };

  const gesture = usePanGesture({
    onActivate: () => {
      startValue.value = progress.value;
    },
    onUpdate: (event) => {
      const next = startValue.value - event.translationY / TRACK_HEIGHT;
      progress.value = Math.min(Math.max(next, 0), 1);
    },
    onDeactivate: () => {
      if (hasSnapPoints) {
        const snappedValue = findClosestValue(progress.value);
        progress.value = withSpring(snappedValue);
        scheduleOnRN(onValueChange, snappedValue);
      } else {
        scheduleOnRN(onValueChange, Math.min(Math.max(progress.value, 0), 1));
      }
    },
  });

  const thumbStyle = useAnimatedStyle(() => ({
    transform: [{ translateY: (1 - progress.value) * TRACK_HEIGHT }],
  }));

  const valueProps = useAnimatedProps(() => {
    const text = hasSnapPoints ? '' : (progress.value * 100).toFixed(0);
    return { text, defaultValue: text };
  });

  return (
    <View style={styles.container}>
      <Text style={[styles.label, { color: labelColor }]}>{label}</Text>
      <View style={styles.trackContainer}>
        <View style={styles.track} />
        {possibleValues?.map((snapValue) => (
          <View
            key={snapValue}
            style={[
              styles.tick,
              { top: (1 - snapValue) * TRACK_HEIGHT + THUMB_SIZE / 2 },
            ]}
          />
        ))}
        <GestureDetector gesture={gesture}>
          <Animated.View style={[styles.thumbHitArea, thumbStyle]}>
            <View style={styles.thumb} />
          </Animated.View>
        </GestureDetector>
      </View>
      <AnimatedValueText
        style={[styles.value, { color: valueColor }]}
        editable={false}
        animatedProps={valueProps}
      />
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
    gap: 5,
    height: SLIDER_HEIGHT + 40,
  },
  label: {
    fontWeight: 'bold',
    fontSize: 12,
  },
  trackContainer: {
    width: 40,
    height: SLIDER_HEIGHT,
    justifyContent: 'center',
    alignItems: 'center',
  },
  track: {
    position: 'absolute',
    width: 4,
    height: '100%',
    backgroundColor: '#111',
    borderRadius: 2,
  },
  tick: {
    position: 'absolute',
    left: '50%',
    width: 12,
    height: 2,
    marginLeft: -6,
    borderRadius: 1,
    backgroundColor: '#fff',
  },
  thumbHitArea: {
    position: 'absolute',
    top: 0,
    width: 40,
    height: THUMB_SIZE,
    justifyContent: 'center',
    alignItems: 'center',
  },
  thumb: {
    width: 30,
    height: 15,
    backgroundColor: '#222',
    borderWidth: 1,
    borderColor: '#fff',
    borderRadius: 2,
    boxShadow: '0 2px 2px rgba(0, 0, 0, 0.5)',
  },
  value: {
    padding: 0,
    fontSize: 10,
    fontVariant: ['tabular-nums'],
  },
});

export default VerticalSlider;
