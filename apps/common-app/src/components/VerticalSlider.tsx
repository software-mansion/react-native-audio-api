import React, { useEffect } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { GestureDetector, Gesture } from 'react-native-gesture-handler';
import Animated, {
  useSharedValue,
  useAnimatedStyle,
} from 'react-native-reanimated';
import { scheduleOnRN } from 'react-native-worklets';

const SLIDER_HEIGHT = 100;
const THUMB_SIZE = 30;
const TRACK_HEIGHT = SLIDER_HEIGHT - THUMB_SIZE;

interface VerticalSliderProps {
  label: string;
  value: number;
  labelColor?: string;
  valueColor?: string;
  onValueChange: (val: number) => void;
}

const VerticalSlider: React.FC<VerticalSliderProps> = ({
  label,
  value,
  labelColor = '#333',
  valueColor = '#555',
  onValueChange,
}) => {
  const progress = useSharedValue(value);
  const startValue = useSharedValue(0);

  useEffect(() => {
    progress.value = value;
  }, [value, progress]);

  const gesture = Gesture.Pan()
    .onStart(() => {
      'worklet';
      startValue.value = progress.value;
    })
    .onUpdate((e) => {
      'worklet';
      const change = -e.translationY / TRACK_HEIGHT;
      const newValue = startValue.value + change;
      progress.value = Math.min(Math.max(newValue, 0), 1);
      scheduleOnRN(onValueChange, progress.value);
    }).onEnd(() => {
      'worklet';
      // progress.value = Math.min(Math.max(progress.value, 0), 1);
      // scheduleOnRN(onValueChange, progress.value);
    });

  const thumbStyle = useAnimatedStyle(() => {
    const translateY = (1 - progress.value) * TRACK_HEIGHT;
    return {
      transform: [{ translateY }],
    };
  });

  return (
    <View style={styles.sliderContainer}>
      <Text style={[styles.sliderLabel, { color: labelColor }]}>{label}</Text>
      <View style={styles.sliderTrackContainer}>
        <View style={styles.sliderTrack} />
        <GestureDetector gesture={gesture}>
          <Animated.View style={[styles.sliderThumbHitArea, thumbStyle]}>
            <View style={styles.sliderThumb} />
          </Animated.View>
        </GestureDetector>
      </View>
      <Text style={[styles.sliderLabel, { color: valueColor }]}>{(value * 100).toFixed(0)}</Text>
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
  },
  sliderTrack: {
    position: 'absolute',
    width: 4,
    height: '100%',
    backgroundColor: '#111',
    borderRadius: 2,
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
