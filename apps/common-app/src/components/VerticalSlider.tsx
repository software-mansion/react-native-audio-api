import React, { useEffect } from 'react';
import { View, Text, StyleSheet, TextInput } from 'react-native';
import { GestureDetector, Gesture } from 'react-native-gesture-handler';
import Animated, {
  useSharedValue,
  useAnimatedStyle,
  withSpring,
  useAnimatedProps,
} from 'react-native-reanimated';
import { runOnJS } from 'react-native-reanimated';
import { scheduleOnRN } from 'react-native-worklets';

const SLIDER_HEIGHT = 100;
const THUMB_SIZE = 30;
const TRACK_HEIGHT = SLIDER_HEIGHT - THUMB_SIZE;

const AText = Animated.createAnimatedComponent(TextInput);

interface VerticalSliderProps {
  label: string;
  value: number;
  labelColor?: string;
  valueColor?: string;
  onValueChange: (val: number) => void;
  possibleValues?: number[];
}

const VerticalSlider: React.FC<VerticalSliderProps> = ({
  label,
  value,
  labelColor = '#333',
  valueColor = '#555',
  onValueChange,
  possibleValues
}) => {
  const progress = useSharedValue(value);
  const startValue = useSharedValue(0);

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

  const handleValueChange = (val: number) => {
    onValueChange(val);
  };

  const gesture = Gesture.Pan()
    .onStart(() => {
      startValue.value = progress.value;
    })
    .onUpdate((e) => {
      const change = -e.translationY / TRACK_HEIGHT;
      const newValue = startValue.value + change;
      const clampedValue = Math.min(Math.max(newValue, 0), 1);
      progress.value = clampedValue;
    })
    .onEnd(() => {
      if (possibleValues && possibleValues.length > 0) {
        const snappedValue = findClosestValue(progress.value);
        progress.value = withSpring(snappedValue);
        scheduleOnRN(handleValueChange, snappedValue);
      } else {
        const finalValue = Math.min(Math.max(progress.value, 0), 1);
        scheduleOnRN(handleValueChange, finalValue);
      }
    });

  const thumbStyle = useAnimatedStyle(() => {
    const translateY = (1 - progress.value) * TRACK_HEIGHT;
    return {
      transform: [{ translateY }],
    };
  });

  const renderTickMarks = () => {
    if (!possibleValues || possibleValues.length === 0) {
      return null;
    }

    return possibleValues.map((val, index) => {
      const position = (1 - val) * TRACK_HEIGHT + THUMB_SIZE / 2;
      return (
        <View
          key={index}
          style={[
            styles.tickMark,
            {
              top: position, // Center the tick mark
            },
          ]}
        />
      );
    });
  };

  const displayProps = useAnimatedProps(() => {
    if (possibleValues && possibleValues.length > 0) {
      return {
        defaultValue: '',
        text: '',
      };
    }

    return {
      defaultValue: (value * 100).toString(),
      text: (progress.value * 100).toFixed(0),
    };
  });

  return (
    <View style={styles.sliderContainer}>
      <Text style={[styles.sliderLabel, { color: labelColor }]}>{label}</Text>
      <View style={styles.sliderTrackContainer}>
        <View style={styles.sliderTrack} />
        {renderTickMarks()}
        <GestureDetector gesture={gesture}>
          <Animated.View style={[styles.sliderThumbHitArea, thumbStyle]}>
            <View style={styles.sliderThumb} />
          </Animated.View>
        </GestureDetector>
      </View>
      <AText style={[styles.sliderValue, { color: valueColor }]} editable={false} animatedProps={displayProps} />
      {/* <Text style={[styles.sliderValue, { color: valueColor }]}>
        {getDisplayValue()}
      </Text> */}
    </View>
  );
};

const styles = StyleSheet.create({
  sliderContainer: {
    alignItems: 'center',
    gap: 5,
    height: SLIDER_HEIGHT + 40,
  },
  sliderLabel: {
    fontWeight: 'bold',
    fontSize: 12,
  },
  sliderTrackContainer: {
    width: 40,
    height: SLIDER_HEIGHT,
    justifyContent: 'center',
    alignItems: 'center',
    position: 'relative',
  },
  sliderTrack: {
    position: 'absolute',
    width: 4,
    height: '100%',
    backgroundColor: '#111',
    borderRadius: 2,
  },
  tickMark: {
    position: 'absolute',
    width: 12,
    height: 2,
    backgroundColor: '#fff',
    borderRadius: 1,
    left: '50%',
    marginLeft: -6, // Center horizontally
  },
  sliderThumbHitArea: {
    position: 'absolute',
    top: 0,
    width: 40,
    height: THUMB_SIZE,
    justifyContent: 'center',
    alignItems: 'center',
  },
  sliderThumb: {
    width: 30,
    height: 15,
    backgroundColor: '#222',
    borderWidth: 1,
    borderColor: '#fff',
    borderRadius: 2,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.5,
    shadowRadius: 2,
  },
  sliderValue: {
    fontSize: 10,
    fontVariant: ['tabular-nums'],
  },
});

export default VerticalSlider;
