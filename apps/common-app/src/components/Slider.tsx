import React from 'react';
import { View, StyleSheet } from 'react-native';
import { FC, useEffect, useCallback } from 'react';
import { GestureDetector, Gesture } from 'react-native-gesture-handler';
import Animated, {
  runOnJS,
  useSharedValue,
  useAnimatedStyle,
} from 'react-native-reanimated';

import { layout, colors } from '../styles';

interface SliderProps {
  value: number;
  onValueChange: (value: number) => void;
  minimumValue: number;
  maximumValue: number;
  step: number;
}

const SLIDER_WIDTH = 300;
const HANDLE_SIZE = 20;
const HANDLE_SPACING = 5;
const MAX_OFFSET = SLIDER_WIDTH - HANDLE_SIZE - 10;

const Slider: FC<SliderProps> = (props) => {
  const { value, onValueChange, minimumValue, maximumValue, step } = props;

  const offset = useSharedValue(0);

  const convertOffsetToValue = useCallback(
    (offsetValue: number) => {
      const newValue =
        minimumValue +
        (offsetValue / MAX_OFFSET) * (maximumValue - minimumValue);
      const steppedValue = Math.round(newValue / step) * step;
      const clampedValue = Math.max(
        minimumValue,
        Math.min(steppedValue, maximumValue)
      );

      onValueChange(clampedValue);
    },
    [minimumValue, maximumValue, step, onValueChange]
  );

  const convertValueToOffset = useCallback(
    (newValue: number) => {
      const fraction =
        (newValue - minimumValue) / (maximumValue - minimumValue);
      return fraction * MAX_OFFSET;
    },
    [minimumValue, maximumValue]
  );

  const pan = Gesture.Pan().onChange((event) => {
    offset.value =
      Math.abs(offset.value) <= MAX_OFFSET
        ? offset.value + event.changeX <= 0
          ? 0
          : offset.value + event.changeX >= MAX_OFFSET
            ? MAX_OFFSET
            : offset.value + event.changeX
        : offset.value;

    runOnJS(convertOffsetToValue)(offset.value);
  });

  const sliderStyle = useAnimatedStyle(() => {
    return {
      transform: [{ translateX: offset.value }],
    };
  });

  useEffect(() => {
    offset.value = convertValueToOffset(value);
  }, [
    value,
    minimumValue,
    maximumValue,
    convertOffsetToValue,
    offset,
    convertValueToOffset,
  ]);

  return (
    <View style={styles.container}>
      <View style={styles.sliderTrack}>
        <GestureDetector gesture={pan}>
          <Animated.View style={[styles.sliderHandle, sliderStyle]} />
        </GestureDetector>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    overflow: 'hidden',
    padding: layout.spacing,
  },
  sliderTrack: {
    width: SLIDER_WIDTH,
    height: HANDLE_SIZE + 10,
    backgroundColor: colors.main,
    borderRadius: 25,
    justifyContent: 'center',
    padding: layout.spacing,
  },
  sliderHandle: {
    width: HANDLE_SIZE,
    height: HANDLE_SIZE,
    backgroundColor: colors.white,
    borderRadius: 20,
    position: 'absolute',
    left: HANDLE_SPACING,
  },
});

export default Slider;
