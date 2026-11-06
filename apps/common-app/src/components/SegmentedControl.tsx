import React from 'react';
import { Pressable, StyleSheet, Text, View } from 'react-native';

import { colors } from '../styles';

export interface SegmentedControlOption<T extends string> {
  value: T;
  label: string;
}

interface SegmentedControlProps<T extends string> {
  options: ReadonlyArray<SegmentedControlOption<T>>;
  value: T;
  onChange: (value: T) => void;
}

const height = 36;
const inset = 3;

/** A capsule row of 2–5 mutually exclusive options. */
function SegmentedControl<T extends string>({
  options,
  value,
  onChange,
}: SegmentedControlProps<T>) {
  return (
    <View style={styles.track} accessibilityRole="tablist">
      {options.map((option) => {
        const isSelected = option.value === value;

        return (
          <Pressable
            key={option.value}
            onPress={() => onChange(option.value)}
            accessibilityRole="tab"
            accessibilityState={{ selected: isSelected }}
            style={[styles.segment, isSelected && styles.selectedSegment]}
          >
            <Text style={[styles.label, isSelected && styles.selectedLabel]}>
              {option.label}
            </Text>
          </Pressable>
        );
      })}
    </View>
  );
}

export default SegmentedControl;

const styles = StyleSheet.create({
  track: {
    height,
    padding: inset,
    borderRadius: height / 2,
    flexDirection: 'row',
    backgroundColor: colors.track,
  },
  segment: {
    flex: 1,
    borderRadius: (height - inset * 2) / 2,
    alignItems: 'center',
    justifyContent: 'center',
  },
  selectedSegment: {
    backgroundColor: 'rgba(238, 240, 255, 0.18)',
  },
  label: {
    fontSize: 14,
    fontWeight: '500',
    color: colors.textSecondary,
  },
  selectedLabel: {
    fontWeight: '600',
    color: colors.textPrimary,
  },
});
