import React, { useCallback, useEffect, useState } from 'react';
import { View, Text, StyleSheet, LayoutChangeEvent } from 'react-native';
import { GestureDetector, Gesture } from 'react-native-gesture-handler';
import Animated, {
  runOnJS,
  withSpring,
  useSharedValue,
  useAnimatedStyle,
} from 'react-native-reanimated';

import { colors } from '../styles';
import Spacer from './Spacer';

interface SliderProps {
  min: number;
  max: number;
  step: number;

  value: number;
  label?: string;
  minLabelWidth?: number;
  onValueChange: (value: number) => void;
}

const handleSize = 20;

function formatSliderValue(value: number, step: number): string {
  const rounded = Math.round(value / step) * step;
  return step < 1 ? rounded.toFixed(2) : String(rounded);
}

function valueToOffset(
  value: number,
  sliderWidth: number,
  min: number,
  max: number
): number {
  'worklet';

  return ((value - min) / (max - min)) * (sliderWidth - handleSize);
}

function offsetToValue(
  offset: number,
  sliderWidth: number,
  min: number,
  max: number
): number {
  'worklet';

  return (offset / (sliderWidth - handleSize)) * (max - min) + min;
}

function roundToStep(value: number, step: number): number {
  'worklet';

  return Math.round(value / step) * step;
}

const Slider: React.FC<SliderProps> = (props) => {
  const { value, onValueChange, min, max, step, label, minLabelWidth } = props;

  const offset = useSharedValue(0);
  const sValue = useSharedValue(0);
  const sliderWidth = useSharedValue(0);
  const [displayValue, setDisplayValue] = useState(() =>
    formatSliderValue(value, step),
  );

  const syncDisplayValue = useCallback(
    (nextValue: number) => {
      setDisplayValue(formatSliderValue(nextValue, step));
    },
    [step],
  );

  useEffect(() => {
    offset.value = valueToOffset(value, sliderWidth.value, min, max);
    sValue.value = value;
    setDisplayValue(formatSliderValue(value, step));
  }, [value, min, max, step, sliderWidth, offset, sValue]);

  const pan = Gesture.Pan()
    .onChange((event) => {
      offset.value = Math.max(
        0,
        Math.min(sliderWidth.value - handleSize, offset.value + event.changeX),
      );

      sValue.value = offsetToValue(offset.value, sliderWidth.value, min, max);
      runOnJS(syncDisplayValue)(sValue.value);
    })
    .onEnd(() => {
      const rounded = roundToStep(sValue.value, step);
      runOnJS(onValueChange)(rounded);
      runOnJS(syncDisplayValue)(rounded);
    });

  const onSliderLayout = (event: LayoutChangeEvent) => {
    sliderWidth.value = event.nativeEvent.layout.width;

    offset.value = valueToOffset(
      value,
      event.nativeEvent.layout.width,
      min,
      max,
    );

    sValue.value = value;
    setDisplayValue(formatSliderValue(value, step));
  };

  const handleStyle = useAnimatedStyle(() => ({
    transform: [{ translateX: offset.value }],
  }));

  const fillStyle = useAnimatedStyle(() => ({
    transform: [
      { translateX: offset.value + handleSize / 2 - sliderWidth.value },
    ],
  }));

  const valueTextStyle = useAnimatedStyle(() => ({
    transform: [
      {
        translateX: withSpring(
          sliderWidth.value > 0 && offset.value < 1.5 * handleSize
            ? 1.5 * handleSize
            : 0,
        ),
      },
    ],
  }));

  return (
    <View style={styles.container}>
      {!!label && (
        <>
          <Text
            style={[
              styles.label,
              !!minLabelWidth && { minWidth: minLabelWidth },
            ]}>
            {label}
          </Text>
          <Spacer.Horizontal size={12} />
        </>
      )}
      <GestureDetector gesture={pan}>
        <View style={styles.slider} onLayout={onSliderLayout}>
          <Animated.View style={[styles.fill, fillStyle]} />
          <Animated.View style={[styles.valueTextContainer, valueTextStyle]}>
            <Text style={styles.valueText}>{displayValue}</Text>
          </Animated.View>
          <Animated.View style={[styles.handle, handleStyle]} />
        </View>
      </GestureDetector>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
  },
  label: {
    fontSize: 16,
    color: colors.white,
  },
  slider: {
    flex: 1,
    borderWidth: 1,
    overflow: 'hidden',
    height: handleSize,
    borderColor: colors.border,
    borderRadius: handleSize / 2,
  },
  fill: {
    position: 'absolute',
    zIndex: -1,
    width: '100%',
    marginTop: -1,
    height: handleSize,
    borderRadius: handleSize / 2,
    backgroundColor: colors.main,
  },
  valueTextContainer: {
    position: 'absolute',
    left: 0,
    right: 0,
    top: 0,
    bottom: 0,
    justifyContent: 'center',
    paddingLeft: 12,
  },
  valueText: {
    fontSize: 16,
    fontWeight: '600',
    color: colors.white,
  },
  handle: {
    marginTop: -1,
    marginLeft: -1,
    width: handleSize,
    height: handleSize,
    borderColor: colors.border,
    borderRadius: handleSize / 2,
    backgroundColor: colors.white,
  },
});

export default Slider;
