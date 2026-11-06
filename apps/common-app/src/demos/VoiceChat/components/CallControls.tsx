import {
  isLiquidGlassSupported,
  LiquidGlassView,
} from '@callstack/liquid-glass';
import React from 'react';
import { Pressable, StyleSheet, Text, View } from 'react-native';

import { AppIcon, Button } from '../../../components';
import { colors } from '../../../styles';

interface CallControlsProps {
  inCall: boolean;
  muted: boolean;
  onStart: () => void;
  onEnd: () => void;
  onToggleMuted: () => void;
}

const buttonSize = 64;

/**
 * Before a call, one "Start conversation" capsule. During one, a glass Mute
 * button (tinted red while muted) and a red End button.
 */
export function CallControls({
  inCall,
  muted,
  onStart,
  onEnd,
  onToggleMuted,
}: CallControlsProps) {
  if (!inCall) {
    return (
      <View style={styles.startRow}>
        <Button title="Start conversation" onPress={onStart} minWidth={220} />
      </View>
    );
  }

  return (
    <View style={styles.row}>
      <View style={styles.control}>
        <Pressable
          onPress={onToggleMuted}
          accessibilityRole="button"
          accessibilityLabel="Mute"
          accessibilityState={{ selected: muted }}
        >
          <LiquidGlassView
            interactive
            colorScheme="dark"
            tintColor={muted ? colors.accent : undefined}
            style={[
              styles.circle,
              !isLiquidGlassSupported &&
                (muted ? styles.mutedFallback : styles.glassFallback),
            ]}
          >
            <AppIcon
              name={muted ? 'mic-off' : 'mic'}
              size={24}
              color={colors.white}
            />
          </LiquidGlassView>
        </Pressable>
        <Text style={styles.label}>{muted ? 'Unmute' : 'Mute'}</Text>
      </View>

      <View style={styles.control}>
        <Pressable
          onPress={onEnd}
          accessibilityRole="button"
          accessibilityLabel="End call"
          style={({ pressed }) => [
            styles.circle,
            styles.endButton,
            pressed && styles.pressed,
          ]}
        >
          <AppIcon name="x" size={26} color={colors.white} />
        </Pressable>
        <Text style={styles.label}>End</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  startRow: {
    height: buttonSize + 26,
    justifyContent: 'center',
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'center',
    gap: 56,
  },
  control: {
    alignItems: 'center',
    gap: 8,
  },
  circle: {
    width: buttonSize,
    height: buttonSize,
    borderRadius: buttonSize / 2,
    alignItems: 'center',
    justifyContent: 'center',
  },
  glassFallback: {
    backgroundColor: colors.surface,
  },
  mutedFallback: {
    backgroundColor: colors.accent,
  },
  endButton: {
    backgroundColor: '#e5484d',
  },
  pressed: {
    opacity: 0.8,
    transform: [{ scale: 0.96 }],
  },
  label: {
    fontSize: 12,
    fontWeight: '500',
    color: colors.textSecondary,
  },
});
