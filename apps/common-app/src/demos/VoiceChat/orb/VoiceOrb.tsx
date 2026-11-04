import { Canvas, Fill, Shader } from '@shopify/react-native-skia';
import React, { useEffect } from 'react';
import { StyleSheet, View } from 'react-native';
import {
  FrameInfo,
  useDerivedValue,
  useFrameCallback,
  useSharedValue,
} from 'react-native-reanimated';

import {
  createVoiceState,
  OrbReadings,
  OrbVoiceState,
  stepVoiceState,
} from './orbAudio';
import { assistantPalette, orbEffect, userPalette } from './orbShader';

interface VoiceOrbProps {
  readings: OrbReadings;
  size: number;
  /** Before a call the orb only breathes, dimmed. */
  isActive: boolean;
}

// Smoothing from rn-audio-shader's voice orb renderer.
const LEVEL_ATTACK_SECONDS = 0.28;
const LEVEL_RELEASE_SECONDS = 0.62;
const PULSE_ATTACK_SECONDS = 0.08;
const PULSE_RELEASE_SECONDS = 0.2;
const BRIGHTNESS_SMOOTH_SECONDS = 0.4;
const MAX_FRAME_DELTA_SECONDS = 1 / 20;
const FLOW_TIME_WRAP_SECONDS = 900;
/** How fast the palette follows a change of speaker. */
const SPEAKER_SMOOTH_SECONDS = 0.35;
/** A voice quieter than this doesn't take the palette over. */
const SPEAKER_THRESHOLD = 0.04;
const IDLE_BRIGHTNESS = 0.15;

type OrbState = {
  user: OrbVoiceState;
  assistant: OrbVoiceState;
  level: number;
  pulse: number;
  brightness: number;
  /** 0 is the assistant's palette, 1 the user's. */
  speaker: number;
  flowTime: number;
};

function approach(
  current: number,
  target: number,
  seconds: number,
  dt: number
) {
  'worklet';
  return current + (target - current) * (1 - Math.exp(-dt / seconds));
}

function frameLevel(voice: OrbVoiceState) {
  'worklet';
  return Math.min(
    1,
    voice.level * 0.45 + voice.smoothedLevel * 1.2 + voice.smoothedPeak * 0.25
  );
}

function mixColor(from: number[], to: number[], amount: number) {
  'worklet';
  return [0, 1, 2].map((i) => from[i] + (to[i] - from[i]) * amount);
}

/**
 * The voice chat's orb: the rn-audio-shader voice orb as a Skia runtime shader.
 * It follows the louder of the two voices and blends its palette toward whoever
 * is speaking.
 */
const VoiceOrb: React.FC<VoiceOrbProps> = ({ readings, size, isActive }) => {
  const isActiveShared = useSharedValue(isActive);
  useEffect(() => {
    isActiveShared.value = isActive;
  }, [isActive, isActiveShared]);

  const state = useSharedValue<OrbState>({
    user: createVoiceState(),
    assistant: createVoiceState(),
    level: 0,
    pulse: 0,
    brightness: 0,
    speaker: 0,
    flowTime: 0,
  });

  useFrameCallback(({ timeSincePreviousFrame }: FrameInfo) => {
    const dt = Math.min(
      MAX_FRAME_DELTA_SECONDS,
      Math.max(0, (timeSincePreviousFrame ?? 16.7) / 1000)
    );
    const frames = dt * 60;
    const next = state.value;
    stepVoiceState(next.user, readings.user.value, frames);
    stepVoiceState(next.assistant, readings.assistant.value, frames);

    const userLevel = frameLevel(next.user);
    const assistantLevel = frameLevel(next.assistant);
    const louder = userLevel > assistantLevel ? next.user : next.assistant;
    const rawLevel = Math.max(userLevel, assistantLevel);
    const rawPulse = Math.min(
      1,
      louder.level * 0.78 +
        louder.peak * 0.44 +
        Math.max(0, rawLevel - next.level) * 1.4
    );
    const rawBrightness = isActiveShared.value
      ? louder.brightness
      : IDLE_BRIGHTNESS;

    next.level = approach(
      next.level,
      rawLevel,
      rawLevel > next.level ? LEVEL_ATTACK_SECONDS : LEVEL_RELEASE_SECONDS,
      dt
    );
    next.pulse = approach(
      next.pulse,
      rawPulse,
      rawPulse > next.pulse ? PULSE_ATTACK_SECONDS : PULSE_RELEASE_SECONDS,
      dt
    );
    next.brightness = approach(
      next.brightness,
      rawBrightness,
      BRIGHTNESS_SMOOTH_SECONDS,
      dt
    );
    if (rawLevel > SPEAKER_THRESHOLD) {
      next.speaker = approach(
        next.speaker,
        userLevel > assistantLevel ? 1 : 0,
        SPEAKER_SMOOTH_SECONDS,
        dt
      );
    }
    next.flowTime += dt * (0.75 + next.level * 0.18 + next.pulse * 0.08);
    if (next.flowTime > FLOW_TIME_WRAP_SECONDS) {
      next.flowTime -= FLOW_TIME_WRAP_SECONDS;
    }
    state.value = { ...next };
  });

  const uniforms = useDerivedValue(() => {
    const { level, pulse, brightness, speaker, flowTime } = state.value;
    return {
      resolution: [size, size],
      time: flowTime,
      amplitude: 0.18 + level * 1.7,
      radius: Math.min(0.9, 0.76 + level * 0.11),
      glow: Math.min(0.58, 0.08 + level * 0.7),
      brightness,
      pulse,
      baseColor: mixColor(assistantPalette.base, userPalette.base, speaker),
      highlightColor: mixColor(
        assistantPalette.highlight,
        userPalette.highlight,
        speaker
      ),
      accentColor: mixColor(
        assistantPalette.accent,
        userPalette.accent,
        speaker
      ),
      softAccentColor: mixColor(
        assistantPalette.softAccent,
        userPalette.softAccent,
        speaker
      ),
      shadowColor: mixColor(
        assistantPalette.shadow,
        userPalette.shadow,
        speaker
      ),
    };
  });

  return (
    <View
      style={[styles.container, { width: size, height: size }]}
      accessible={false}
      importantForAccessibility="no-hide-descendants"
    >
      <Canvas style={StyleSheet.absoluteFill}>
        <Fill>
          <Shader source={orbEffect} uniforms={uniforms} />
        </Fill>
      </Canvas>
    </View>
  );
};

export default VoiceOrb;

const styles = StyleSheet.create({
  container: {
    alignSelf: 'center',
  },
});
