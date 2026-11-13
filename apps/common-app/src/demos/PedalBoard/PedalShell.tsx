import React, { PropsWithChildren } from 'react';
import { Pressable, StyleSheet, Text, View } from 'react-native';
import Svg, { Line, Path } from 'react-native-svg';

import { colors } from '../../styles';

type PedalShellProps = PropsWithChildren<{
  name: string;
  /** Accent used for the side stripe, the lit LED and the active controls. */
  color: string;
  isActive: boolean;
  onToggle: () => void;
}>;

const stompSize = 44;

/**
 * Card chrome shared by every pedal: a colour stripe, the brand / model header,
 * and the LED + footswitch that toggles the effect. Children are laid out in a
 * row as the pedal's controls.
 */
const PedalShell: React.FC<PedalShellProps> = ({
  name,
  color,
  isActive,
  onToggle,
  children,
}) => (
  <View
    style={[
      styles.body,
      { borderColor: isActive ? `${color}66` : colors.listSeparator },
    ]}
  >
    <View style={[styles.stripe, { backgroundColor: color }]} />
    <View style={styles.header}>
      <View>
        <Text style={styles.brand}>RN AUDIO API</Text>
        <Text
          style={[styles.model, { color: isActive ? color : colors.white }]}
        >
          {name}
        </Text>
      </View>
      <View style={styles.powerRow}>
        <View
          style={[
            styles.led,
            {
              backgroundColor: isActive ? color : colors.recess,
              shadowColor: color,
              shadowOpacity: isActive ? 0.9 : 0,
            },
          ]}
        />
        <Pressable
          onPress={onToggle}
          hitSlop={8}
          accessibilityRole="switch"
          accessibilityLabel={`${name} power`}
          accessibilityState={{ checked: isActive }}
          style={({ pressed }) => [
            styles.stomp,
            pressed && styles.stompPressed,
            isActive && { borderColor: `${color}99` },
          ]}
        >
          <PowerGlyph color={isActive ? color : colors.textMuted} size={20} />
        </Pressable>
      </View>
    </View>
    <View style={styles.controls}>{children}</View>
  </View>
);

export default PedalShell;

/**
 * The icon font has no power symbol, so the footswitch draws Lucide's `power`
 * inline.
 */
const PowerGlyph: React.FC<{ color: string; size: number }> = ({
  color,
  size,
}) => (
  <Svg
    width={size}
    height={size}
    viewBox="0 0 24 24"
    fill="none"
    stroke={color}
    strokeWidth={2.6}
    strokeLinecap="round"
    strokeLinejoin="round"
  >
    <Path d="M12 2v10" />
    <Path d="M18.4 6.6a9 9 0 1 1-12.77.04" />
    <Line x1={12} y1={2} x2={12} y2={2} />
  </Svg>
);

const styles = StyleSheet.create({
  body: {
    backgroundColor: colors.surface,
    borderRadius: 22,
    borderWidth: 1,
    paddingVertical: 16,
    paddingHorizontal: 18,
    gap: 14,
    overflow: 'hidden',
  },
  stripe: {
    position: 'absolute',
    left: 0,
    top: 0,
    bottom: 0,
    width: 4,
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  brand: {
    fontSize: 10,
    fontWeight: '700',
    color: colors.textMuted,
    letterSpacing: 2,
  },
  model: {
    fontSize: 22,
    fontWeight: '800',
    letterSpacing: 1,
    fontStyle: 'italic',
  },
  powerRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
  },
  led: {
    width: 10,
    height: 10,
    borderRadius: 5,
    shadowOffset: { width: 0, height: 0 },
    shadowRadius: 6,
  },
  stomp: {
    width: stompSize,
    height: stompSize,
    borderRadius: stompSize / 2,
    backgroundColor: colors.surfacePressed,
    borderWidth: 1,
    borderColor: colors.listSeparator,
    alignItems: 'center',
    justifyContent: 'center',
  },
  stompPressed: {
    transform: [{ scale: 0.92 }],
  },
  controls: {
    flexDirection: 'row',
    justifyContent: 'space-evenly',
    alignItems: 'flex-start',
  },
});
