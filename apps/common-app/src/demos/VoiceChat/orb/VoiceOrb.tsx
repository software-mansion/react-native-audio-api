import { Canvas, Fill, Shader } from '@shopify/react-native-skia';
import React, { useEffect, useState } from 'react';
import {
  AccessibilityInfo,
  AppState,
  PixelRatio,
  Pressable,
  StyleSheet,
  View,
} from 'react-native';
import Animated, {
  FrameInfo,
  useDerivedValue,
  useFrameCallback,
  useSharedValue,
} from 'react-native-reanimated';

import { OrbColor, OrbPalette, orbPalettes, OrbState } from './orbPalettes';
import { orbEdgeWidth, orbEffect } from './orbShader';
import { useOrbTapAnimation } from './useOrbTapAnimation';

interface VoiceOrbProps {
  state: OrbState;
  /** Speech detected on the microphone. Only this moves the listening orb. */
  hearing: boolean;
  size: number;
  onPress: () => void;
  accessibilityLabel: string;
  disabled?: boolean;
}

const MAX_FRAME_DELTA_SECONDS = 0.05;
/** How fast colours and motion follow a change of state. */
const BLEND_SECONDS = 0.18;
const TIME_WRAP_SECONDS = 900;

type OrbAppearance = OrbPalette & {
  time: number;
  listening: number;
  speaking: number;
};

const paletteChannels = ['primary', 'accent', 'highlight'] as const;

function approachColor(current: OrbColor, target: OrbColor, blend: number) {
  'worklet';
  for (let i = 0; i < 3; i += 1) {
    current[i] += (target[i] - current[i]) * blend;
  }
}

function isAt(appearance: OrbAppearance, target: OrbAppearance) {
  'worklet';
  return (
    appearance.listening === target.listening &&
    appearance.speaking === target.speaking &&
    paletteChannels.every((channel) =>
      appearance[channel].every((value, i) => value === target[channel][i])
    )
  );
}

/** Whether the system asks for reduced motion; the orb then holds still. */
function useReduceMotion() {
  const [reduceMotion, setReduceMotion] = useState(false);
  useEffect(() => {
    let mounted = true;
    const subscription = AccessibilityInfo.addEventListener(
      'reduceMotionChanged',
      setReduceMotion
    );
    AccessibilityInfo.isReduceMotionEnabled()
      .then((enabled) => {
        if (mounted) {
          setReduceMotion(enabled);
        }
      })
      .catch(() => undefined);
    return () => {
      mounted = false;
      subscription.remove();
    };
  }, []);
  return reduceMotion;
}

function useForeground() {
  const [foreground, setForeground] = useState(
    AppState.currentState === 'active'
  );
  useEffect(() => {
    const subscription = AppState.addEventListener('change', (next) => {
      setForeground(next === 'active');
    });
    return () => subscription.remove();
  }, []);
  return foreground;
}

/**
 * The voice chat's orb: the aec-workshop promo orb as a Skia runtime shader.
 * Its palette follows the session state, it stirs while someone is talking, and
 * tapping it starts or ends the conversation.
 */
const VoiceOrb: React.FC<VoiceOrbProps> = ({
  state,
  hearing,
  size,
  onPress,
  accessibilityLabel,
  disabled = false,
}) => {
  const reduceMotion = useReduceMotion();
  const foreground = useForeground();
  const { animatedStyle, pulse } = useOrbTapAnimation();

  const target = useSharedValue<OrbAppearance>({
    ...orbPalettes[state],
    time: 0,
    listening: 0,
    speaking: 0,
  });
  const stationary = useSharedValue(reduceMotion);
  useEffect(() => {
    const palette = orbPalettes[state];
    // Listening colours stay on while the mic is open; motion needs speech.
    const hearingSpeech = !reduceMotion && state === 'listening' && hearing;
    target.value = {
      primary: [...palette.primary],
      accent: [...palette.accent],
      highlight: [...palette.highlight],
      time: 0,
      listening: hearingSpeech ? 1 : 0,
      speaking: state === 'speaking' ? 1 : 0,
    };
    stationary.value = reduceMotion;
  }, [state, hearing, reduceMotion, target, stationary]);

  const appearance = useSharedValue<OrbAppearance>({
    primary: [...orbPalettes[state].primary],
    accent: [...orbPalettes[state].accent],
    highlight: [...orbPalettes[state].highlight],
    time: 0,
    listening: 0,
    speaking: 0,
  });

  const frame = useFrameCallback(({ timeSincePreviousFrame }: FrameInfo) => {
    const current = appearance.value;
    const goal = target.value;
    if (stationary.value && isAt(current, goal)) {
      return;
    }
    const dt = Math.min(
      MAX_FRAME_DELTA_SECONDS,
      Math.max(0, (timeSincePreviousFrame ?? 16.7) / 1000)
    );
    const blend = stationary.value ? 1 : 1 - Math.exp(-dt / BLEND_SECONDS);
    current.listening += (goal.listening - current.listening) * blend;
    current.speaking += (goal.speaking - current.speaking) * blend;
    for (const channel of paletteChannels) {
      approachColor(current[channel], goal[channel], blend);
    }
    if (!stationary.value) {
      current.time =
        (current.time +
          dt * (0.75 + current.listening * 0.45 + current.speaking * 1.8)) %
        TIME_WRAP_SECONDS;
    }
    appearance.value = { ...current };
  }, false);
  useEffect(() => {
    frame.setActive(foreground);
  }, [foreground, frame]);

  const edgeWidth = orbEdgeWidth(size * PixelRatio.get());
  const uniforms = useDerivedValue(() => {
    const { time, listening, speaking, primary, accent, highlight } =
      appearance.value;
    return {
      resolution: [size, size],
      time,
      listening,
      speaking,
      edgeWidth,
      primaryColor: primary,
      accentColor: accent,
      highlightColor: highlight,
    };
  });

  return (
    <Pressable
      accessibilityRole="button"
      accessibilityLabel={accessibilityLabel}
      accessibilityHint="Starts or ends the conversation."
      accessibilityState={{ disabled }}
      disabled={disabled}
      onPress={() => {
        pulse();
        onPress();
      }}
      style={[styles.control, { width: size, height: size }]}
    >
      <Animated.View
        pointerEvents="none"
        style={[StyleSheet.absoluteFill, animatedStyle]}
      >
        <View
          style={StyleSheet.absoluteFill}
          accessible={false}
          importantForAccessibility="no-hide-descendants"
        >
          <Canvas style={StyleSheet.absoluteFill}>
            <Fill>
              <Shader source={orbEffect} uniforms={uniforms} />
            </Fill>
          </Canvas>
        </View>
      </Animated.View>
    </Pressable>
  );
};

export default VoiceOrb;

const styles = StyleSheet.create({
  control: {
    alignSelf: 'center',
    borderRadius: 999,
  },
});
