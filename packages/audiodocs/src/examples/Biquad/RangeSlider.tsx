import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

type RangeSliderProps = {
  label: string;
  value: number;
  min: number;
  max: number;
  step: number;
  unit?: string;
  onChange: (v: number) => void;
};

const RangeSlider: React.FC<RangeSliderProps> = ({ label, value, min, max, step, unit = '', onChange }) => {
  return (
    <View style={styles.container}>
      <Text style={styles.label}>
        {label}: {value.toFixed(1)} {unit}
      </Text>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) => onChange(Number(e.target.value))}
        style={styles.slider}
      />
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    marginTop: 8,
    width: '100%',
  },
  label: {
    color: '#fcfcff',
    marginBottom: 4,
  },
  slider: {
    width: '100%',
    color: '#6676aa',
  },
});

export default RangeSlider;
