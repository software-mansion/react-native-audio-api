import React from 'react';
import { StyleSheet, View } from 'react-native';

import { Button, RoundButton } from '../../../components';
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
 * button (tinted with the accent while muted) and a red End button.
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
      <RoundButton
        variant="glass"
        icon={muted ? 'mic-off' : 'mic'}
        label={muted ? 'Unmute' : 'Mute'}
        accessibilityLabel="Mute"
        selected={muted}
        color={colors.accent}
        onPress={onToggleMuted}
        size={buttonSize}
        iconSize={24}
      />
      <RoundButton
        icon="x"
        label="End"
        accessibilityLabel="End call"
        color={colors.danger}
        onPress={onEnd}
        size={buttonSize}
        iconSize={26}
      />
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
});
