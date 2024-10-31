import React, { useEffect } from 'react';
import {
  View,
  Text,
  TextInput,
  StyleSheet,
  LayoutChangeEvent,
} from 'react-native';
import { GestureDetector, Gesture } from 'react-native-gesture-handler';
import Animated, {
  runOnJS,
  withSpring,
  useSharedValue,
  useAnimatedProps,
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

const AnimatedText = Animated.createAnimatedComponent(TextInput);

const Slider: React.FC<SliderProps> = (props) => {
  const { value, onValueChange, min, max, step, label, minLabelWidth } = props;

  const offset = useSharedValue(0);
  const sValue = useSharedValue(0);
  const sliderWidth = useSharedValue(0);

  useEffect(() => {
    offset.value = valueToOffset(value, sliderWidth.value, min, max);
    sValue.value = value;
  }, [value, min, max, sliderWidth, offset, sValue]);

  const pan = Gesture.Pan()
    .onChange((event) => {
      offset.value = Math.max(
        0,
        Math.min(sliderWidth.value - handleSize, offset.value + event.changeX)
      );

      sValue.value = offsetToValue(offset.value, sliderWidth.value, min, max);
    })
    .onEnd(() => {
      runOnJS(onValueChange)(roundToStep(sValue.value, step));
    });

  const onSliderLayout = (event: LayoutChangeEvent) => {
    sliderWidth.value = event.nativeEvent.layout.width;

    offset.value = valueToOffset(
      value,
      event.nativeEvent.layout.width,
      min,
      max
    );

    sValue.value = value;
  };

  const handleStyle = useAnimatedStyle(() => ({
    transform: [{ translateX: offset.value }],
  }));

  const fillStyle = useAnimatedStyle(() => ({
    transform: [
      { translateX: offset.value + handleSize / 2 - sliderWidth.value },
    ],
  }));

  const valueTextProps = useAnimatedProps(() => ({
    text: `${step < 1 ? roundToStep(sValue.value, step).toFixed(2) : roundToStep(sValue.value, step)}`,
    defaultValue: `${roundToStep(sValue.value, step)}`,
  }));

  const valueTextStyle = useAnimatedStyle(() => ({
    transform: [
      {
        translateX: withSpring(
          sliderWidth.value > 0 && offset.value < 1.5 * handleSize
            ? 1.5 * handleSize
            : 0
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
            ]}
          >
            {label}
          </Text>
          <Spacer.Horizontal size={12} />
        </>
      )}
      <GestureDetector gesture={pan}>
        <View style={styles.slider} onLayout={onSliderLayout}>
          <Animated.View style={[styles.fill, fillStyle]} />
          <AnimatedText
            editable={false}
            animatedProps={valueTextProps}
            style={[styles.valueText, valueTextStyle]}
          />
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
  valueText: {
    fontSize: 16,
    fontWeight: '600',
    color: colors.white,
    position: 'absolute',
    left: 0,
    right: 0,
    top: 0,
    bottom: 0,
    paddingLeft: 12,
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
