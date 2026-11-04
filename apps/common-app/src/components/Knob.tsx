import {
  Canvas,
  Circle,
  Group,
  Line,
  Path,
  Skia,
  rect,
  vec,
} from '@shopify/react-native-skia';
import React, { useEffect, useMemo } from 'react';
import { StyleSheet, Text, TextInput, View } from 'react-native';
import { GestureDetector, usePanGesture } from 'react-native-gesture-handler';
import Animated, {
  useAnimatedProps,
  useDerivedValue,
  useSharedValue,
  withSpring,
} from 'react-native-reanimated';
import { scheduleOnRN } from 'react-native-worklets';

import { colors } from '../styles';

const AnimatedTextInput = Animated.createAnimatedComponent(TextInput);

// A classic knob sweeps 270° starting at the 7 o'clock position.
const ARC_START_DEGREES = 135;
const ARC_SWEEP_DEGREES = 270;
const TRACK_WIDTH = 5;
// Pixels of vertical drag that move the knob through its full range.
const DRAG_RANGE_PX = 160;

interface KnobProps {
  label: string;
  /** Normalised 0..1 position. */
  value: number;
  onValueChange: (value: number) => void;
  color?: string;
  size?: number;
  formatValue?: (value: number) => string;
}

const defaultFormatValue = (value: number) => {
  'worklet';
  return Math.round(value * 100).toString();
};

/**
 * A rotary control driven by a vertical drag. The readout under the knob is an
 * animated TextInput so it tracks the drag without re-rendering on every
 * frame.
 */
const Knob: React.FC<KnobProps> = ({
  label,
  value,
  onValueChange,
  color = colors.accent,
  size = 64,
  formatValue = defaultFormatValue,
}) => {
  const progress = useSharedValue(value);
  const dragStartValue = useSharedValue(value);

  useEffect(() => {
    progress.value = withSpring(value, { damping: 20, stiffness: 180 });
  }, [value, progress]);

  const center = size / 2;
  const trackRadius = center - TRACK_WIDTH;
  const faceRadius = trackRadius - TRACK_WIDTH - 4;

  const arcPath = useMemo(() => {
    const builder = Skia.PathBuilder.Make();
    builder.addArc(
      rect(
        center - trackRadius,
        center - trackRadius,
        trackRadius * 2,
        trackRadius * 2
      ),
      ARC_START_DEGREES,
      ARC_SWEEP_DEGREES
    );
    return builder.build();
  }, [center, trackRadius]);

  const pointerTransform = useDerivedValue(() => {
    const degrees = -ARC_SWEEP_DEGREES / 2 + progress.value * ARC_SWEEP_DEGREES;
    return [{ rotate: (degrees * Math.PI) / 180 }];
  });

  const pan = usePanGesture({
    onActivate: () => {
      dragStartValue.value = progress.value;
    },
    onUpdate: (event) => {
      const next = dragStartValue.value - event.translationY / DRAG_RANGE_PX;
      progress.value = Math.min(1, Math.max(0, next));
      scheduleOnRN(onValueChange, progress.value);
    },
  });

  const valueTextProps = useAnimatedProps(() => ({
    text: formatValue(progress.value),
    defaultValue: formatValue(progress.value),
  }));

  return (
    <View style={styles.container}>
      <GestureDetector gesture={pan}>
        <View style={{ width: size, height: size }}>
          <Canvas style={{ width: size, height: size }}>
            <Path
              path={arcPath}
              style="stroke"
              strokeWidth={TRACK_WIDTH}
              strokeCap="round"
              color={colors.track}
            />
            <Path
              path={arcPath}
              style="stroke"
              strokeWidth={TRACK_WIDTH}
              strokeCap="round"
              color={color}
              start={0}
              end={progress}
            />
            <Circle
              cx={center}
              cy={center}
              r={faceRadius}
              color={colors.surfacePressed}
            />
            <Circle
              cx={center}
              cy={center}
              r={faceRadius}
              style="stroke"
              strokeWidth={1}
              color="#ffffff1f"
            />
            <Group origin={vec(center, center)} transform={pointerTransform}>
              <Line
                p1={vec(center, center - faceRadius + 5)}
                p2={vec(center, center - faceRadius * 0.45)}
                strokeWidth={3}
                strokeCap="round"
                color={colors.white}
              />
            </Group>
          </Canvas>
        </View>
      </GestureDetector>
      <Text style={styles.label}>{label}</Text>
      <AnimatedTextInput
        editable={false}
        animatedProps={valueTextProps}
        style={[styles.value, { color }]}
      />
    </View>
  );
};

export default Knob;

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
    gap: 2,
  },
  label: {
    marginTop: 4,
    color: colors.textMuted,
    fontSize: 10,
    fontWeight: '700',
    letterSpacing: 1.4,
    textTransform: 'uppercase',
  },
  value: {
    fontSize: 13,
    fontWeight: '700',
    fontVariant: ['tabular-nums'],
    padding: 0,
    textAlign: 'center',
    minWidth: 40,
  },
});
