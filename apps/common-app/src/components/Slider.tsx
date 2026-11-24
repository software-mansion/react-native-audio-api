import NativeSlider from '@react-native-community/slider';
import React, { useEffect, useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { colors } from '../styles';

interface SliderProps {
  min: number;
  max: number;
  step: number;

  value: number;
  label?: string;
  /**
   * `logarithmic` spreads a wide range evenly by ratio (each doubling covers
   * the same track distance). Requires `min > 0`.
   */
  scale?: SliderScale;
  /** Called once the drag ends, with the value rounded to `step`. */
  onValueChange: (value: number) => void;
}

type SliderScale = 'linear' | 'logarithmic';

function formatSliderValue(value: number, step: number): string {
  const rounded = roundToStep(value, step);
  return step < 1 ? rounded.toFixed(2) : String(rounded);
}

function roundToStep(value: number, step: number): number {
  return Math.round(value / step) * step;
}

function valueToProgress(value: number, min: number, max: number): number {
  const progress = Math.log(value / min) / Math.log(max / min);
  return Math.min(1, Math.max(0, progress));
}

function progressToValue(progress: number, min: number, max: number): number {
  return min * Math.pow(max / min, progress);
}

/**
 * The system slider (UISlider on iOS, with the liquid glass thumb on iOS 26)
 * under a row holding the label and the live value.
 *
 * A logarithmic slider runs the native control over 0–1 and maps that progress
 * to the value, so it rounds to `step` itself instead of letting the native
 * control snap.
 */
const Slider: React.FC<SliderProps> = ({
  value,
  onValueChange,
  min,
  max,
  step,
  label,
  scale = 'linear',
}) => {
  const isLogarithmic = scale === 'logarithmic';
  const [displayValue, setDisplayValue] = useState(value);

  useEffect(() => {
    setDisplayValue(value);
  }, [value]);

  const fromNative = (sliderValue: number) =>
    isLogarithmic ? progressToValue(sliderValue, min, max) : sliderValue;

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        {!!label && <Text style={styles.label}>{label}</Text>}
        <Text style={styles.value}>
          {formatSliderValue(displayValue, step)}
        </Text>
      </View>
      <NativeSlider
        style={styles.slider}
        value={isLogarithmic ? valueToProgress(value, min, max) : value}
        minimumValue={isLogarithmic ? 0 : min}
        maximumValue={isLogarithmic ? 1 : max}
        step={isLogarithmic ? 0 : step}
        minimumTrackTintColor={colors.accent}
        maximumTrackTintColor={colors.track}
        tapToSeek
        accessibilityLabel={label}
        onValueChange={(sliderValue) =>
          setDisplayValue(fromNative(sliderValue))
        }
        onSlidingComplete={(sliderValue) =>
          onValueChange(roundToStep(fromNative(sliderValue), step))
        }
      />
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    alignSelf: 'stretch',
  },
  header: {
    flexDirection: 'row',
    alignItems: 'baseline',
    justifyContent: 'space-between',
    marginHorizontal: 4,
  },
  label: {
    fontSize: 15,
    color: colors.textSecondary,
  },
  value: {
    marginLeft: 'auto',
    fontSize: 15,
    fontWeight: '600',
    fontVariant: ['tabular-nums'],
    color: colors.textPrimary,
  },
  slider: {
    height: 40,
  },
});

export default Slider;
