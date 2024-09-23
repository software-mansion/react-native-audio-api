import { FC, useState, useEffect, useCallback } from 'react';
import { StyleSheet, View, StyleProp, ViewStyle } from 'react-native';
import { Gesture, GestureDetector } from 'react-native-gesture-handler';
import Animated, {
  clamp,
  useAnimatedStyle,
  useSharedValue,
  runOnJS,
} from 'react-native-reanimated';

import { colorShades, layout } from '../styles';

const hitSlop = { top: 20, left: 20, right: 20, bottom: 20 };

interface CustomSliderProps {
  value: number;
  onValueChange: (value: number) => void;
  minimumValue: number;
  maximumValue: number;
  step: number;
  style?: StyleProp<ViewStyle>;
}

const CustomSlider: FC<CustomSliderProps> = (props) => {
  const { value, onValueChange, minimumValue, maximumValue, step, style } =
    props;
  const x = useSharedValue(0);
  const [sliderWidth, setSliderWidth] = useState<number>(300);

  const convertXToValue = useCallback(
    (xValue: number) => {
      const fraction = xValue / sliderWidth;
      const newValue = minimumValue + fraction * (maximumValue - minimumValue);
      runOnJS(onValueChange)(Math.round(newValue / step) * step);
    },
    [sliderWidth, minimumValue, maximumValue, step, onValueChange]
  );

  const convertValueToX = useCallback(
    (newValue: number) => {
      const fraction =
        (newValue - minimumValue) / (maximumValue - minimumValue);
      x.value = fraction * sliderWidth;
    },
    [sliderWidth, minimumValue, maximumValue, x]
  );

  const panGesture = Gesture.Pan()
    .averageTouches(true)
    .onChange((ev) => {
      x.value = clamp((x.value += ev.changeX), 0, sliderWidth);
      runOnJS(convertXToValue)(x.value);
    })
    .onEnd(() => {
      runOnJS(convertXToValue)(x.value);
    });

  const animatedStyle = useAnimatedStyle(() => {
    return {
      transform: [
        {
          translateX: x.value,
        },
      ],
    };
  });

  useEffect(() => {
    if (style && 'width' in style) {
      setSliderWidth(style.width as number);
    }

    runOnJS(convertValueToX)(value);
  }, [style, value, minimumValue, maximumValue, convertValueToX]);

  return (
    <View style={[styles.default, style]}>
      <GestureDetector gesture={panGesture}>
        <View
          style={[styles.slider, styles.default, { width: sliderWidth }]}
          hitSlop={hitSlop}
        >
          <Animated.View style={[styles.progress, { width: x }]} />
          <Animated.View style={[styles.knob, animatedStyle]} />
        </View>
      </GestureDetector>
    </View>
  );
};

const styles = StyleSheet.create({
  default: {
    width: '80%',
  },
  knob: {
    width: layout.knobSize,
    height: layout.knobSize,
    borderRadius: layout.knobSize / 2,
    borderWidth: layout.knobSize / 2,
    borderColor: colorShades.blue.base,
    position: 'absolute',
    left: -layout.knobSize / 2,
  },
  slider: {
    backgroundColor: colorShades.blue.light,
    height: 5,
    justifyContent: 'center',
  },
  progress: {
    height: 5,
    backgroundColor: colorShades.blue.dark,
    position: 'absolute',
  },
});

export default CustomSlider;
