import React, { FC } from 'react';
import { Pressable, StyleSheet, Text, View } from 'react-native';

import { colors } from '../styles';
import AppIcon, { type AppIconName } from './AppIcon';

interface RoundButtonProps {
  /** A glyph from the icon font, or `stop` for a filled square the font lacks. */
  icon: AppIconName | 'stop';
  onPress: () => void;
  /** Small caption rendered under the circle. */
  label?: string;
  size?: number;
  color?: string;
  /**
   * `solid` fills the circle with `color`; `outline` keeps a surface circle
   * with a tinted glyph.
   */
  variant?: 'solid' | 'outline';
  disabled?: boolean;
}

/**
 * A circular icon button with a glow in its colour, for transport-style
 * controls.
 */
const RoundButton: FC<RoundButtonProps> = ({
  icon,
  onPress,
  label,
  size = 64,
  color = colors.accent,
  variant = 'solid',
  disabled = false,
}) => {
  const isSolid = variant === 'solid';

  return (
    <View style={styles.wrapper}>
      <Pressable
        disabled={disabled}
        onPress={onPress}
        accessibilityRole="button"
        accessibilityLabel={label}
        accessibilityState={{ disabled }}
        style={({ pressed }) => [
          styles.button,
          {
            width: size,
            height: size,
            borderRadius: size / 2,
            backgroundColor: isSolid ? color : colors.surfacePressed,
            borderColor: isSolid ? color : colors.listSeparator,
            shadowColor: color,
            shadowOpacity: isSolid ? 0.45 : 0,
            opacity: disabled ? 0.35 : 1,
            transform: [{ scale: pressed ? 0.92 : 1 }],
          },
        ]}
      >
        {icon === 'stop' ? (
          <View
            style={[
              styles.stopSquare,
              {
                width: size * 0.34,
                height: size * 0.34,
                backgroundColor: isSolid ? colors.white : color,
              },
            ]}
          />
        ) : (
          <AppIcon
            name={icon}
            color={isSolid ? colors.white : color}
            size={size * 0.42}
          />
        )}
      </Pressable>
      {label ? <Text style={styles.label}>{label}</Text> : null}
    </View>
  );
};

export default RoundButton;

const styles = StyleSheet.create({
  wrapper: {
    alignItems: 'center',
    gap: 8,
  },
  button: {
    alignItems: 'center',
    justifyContent: 'center',
    borderWidth: 1,
    shadowOffset: { width: 0, height: 6 },
    shadowRadius: 14,
    elevation: 6,
  },
  stopSquare: {
    borderRadius: 5,
  },
  label: {
    color: colors.textMuted,
    fontSize: 11,
    fontWeight: '700',
    letterSpacing: 1.2,
    textTransform: 'uppercase',
  },
});
