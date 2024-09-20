import { FC } from 'react';
import { StyleSheet, View } from 'react-native';
import { Gesture, GestureDetector } from 'react-native-gesture-handler';
import Animated, {
  Extrapolation,
  clamp,
  interpolate,
  measure,
  useAnimatedRef,
  useAnimatedStyle,
  useSharedValue,
  withSpring,
} from 'react-native-reanimated';

import { colorShades, layout } from '../styles';
import Container from './Container';

const hitSlop = { top: 20, left: 20, right: 20, bottom: 20 };

interface CustomSliderProps {
  value?: number;
  onValueChange?: (value: number) => void;
  minimumValue?: number;
  maximumValue?: number;
  step?: number;
}

const CustomSlider: FC<CustomSliderProps> = (props) => {
  const { value, onValueChange, minimumValue, maximumValue, step } = props;
  const x = useSharedValue(0);
  const knobScale = useSharedValue(1);

  const tapGesture = Gesture.Tap()
    .maxDuration(100000)
    .onStart(() => {
      knobScale.value = withSpring(1);
    })
    .onEnd(() => {
      knobScale.value = withSpring(0);
    });

  const aRef = useAnimatedRef<View>();

  const panGesture = Gesture.Pan()
    .averageTouches(true)
    .onChange((ev) => {
      const size = measure(aRef);
      x.value = clamp((x.value += ev.changeX), 0, size.width);
    })
    .onEnd(() => {
      knobScale.value = withSpring(1);
    });
  const gestures = Gesture.Simultaneous(tapGesture, panGesture);
  const animatedStyle = useAnimatedStyle(() => {
    return {
      borderWidth: interpolate(
        knobScale.value,
        [0, 1],
        [layout.knobSize / 2, 2],
        Extrapolation.CLAMP
      ),
      transform: [
        {
          translateX: x.value,
        },
        {
          scale: knobScale.value,
        },
      ],
    };
  });

  return (
    <Container centered={true}>
      <GestureDetector gesture={gestures}>
        <View ref={aRef} style={styles.slider} hitSlop={hitSlop}>
          <Animated.View style={[styles.progress, { width: x }]} />
          <Animated.View style={[styles.knob, animatedStyle]} />
        </View>
      </GestureDetector>
    </Container>
  );
};

const styles = StyleSheet.create({
  knob: {
    width: layout.knobSize,
    height: layout.knobSize,
    borderRadius: layout.knobSize / 2,
    backgroundColor: colorShades.white.base,
    borderWidth: layout.knobSize / 2,
    borderColor: colorShades.blue.base,
    position: 'absolute',
    left: -layout.knobSize / 2,
  },
  slider: {
    width: '80%',
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
