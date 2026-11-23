import React, { FC } from 'react';
import {
  ActivityIndicator,
  Pressable,
  StyleSheet,
  Text,
  View,
} from 'react-native';

import { colors, typography } from '../styles';
import AppIcon, { type AppIconName } from './AppIcon';
import GlassSurface from './GlassSurface';

interface RoundButtonProps {
  /** A glyph from the icon font, or `stop` for a filled square the font lacks. */
  icon: AppIconName | 'stop';
  onPress: () => void;
  /** Small caption rendered under the circle. */
  label?: string;
  /** Screen-reader name; defaults to `label`. */
  accessibilityLabel?: string;
  size?: number;
  /** Defaults to 42% of `size`. */
  iconSize?: number;
  color?: string;
  /** Defaults to `color` on `outline`, `textPrimary` otherwise. */
  iconColor?: string;
  /**
   * `solid` fills the circle with `color` and glows; `outline` keeps a surface
   * circle with a tinted glyph; `glass` is a liquid glass circle, tinted with
   * `color` only while `selected`.
   */
  variant?: 'solid' | 'outline' | 'glass';
  selected?: boolean;
  /** Replaces the glyph with a spinner. */
  loading?: boolean;
  disabled?: boolean;
}

/** A circular icon button for transport-style and toolbar controls. */
const RoundButton: FC<RoundButtonProps> = ({
  icon,
  onPress,
  label,
  accessibilityLabel = label,
  size = 64,
  iconSize = size * 0.42,
  color = colors.accent,
  iconColor,
  variant = 'solid',
  selected = false,
  loading = false,
  disabled = false,
}) => {
  const isSolid = variant === 'solid';
  const isGlass = variant === 'glass';
  const glyphColor =
    isGlass && disabled
      ? colors.textMuted
      : (iconColor ?? (variant === 'outline' ? color : colors.textPrimary));
  const circle = { width: size, height: size, borderRadius: size / 2 };

  const glyph = loading ? (
    <ActivityIndicator color={glyphColor} />
  ) : icon === 'stop' ? (
    <View
      style={[
        styles.stopSquare,
        {
          width: size * 0.34,
          height: size * 0.34,
          backgroundColor: glyphColor,
        },
      ]}
    />
  ) : (
    <AppIcon name={icon} color={glyphColor} size={iconSize} />
  );

  return (
    <View style={styles.wrapper}>
      <Pressable
        disabled={disabled}
        onPress={onPress}
        accessibilityRole="button"
        accessibilityLabel={accessibilityLabel}
        accessibilityState={{ disabled, selected }}
        style={({ pressed }) =>
          isGlass
            ? undefined
            : [
                styles.button,
                circle,
                {
                  backgroundColor: isSolid ? color : colors.surfacePressed,
                  borderColor: isSolid ? color : colors.track,
                  shadowColor: color,
                  shadowOpacity: isSolid ? 0.45 : 0,
                  transform: [{ scale: pressed ? 0.92 : 1 }],
                },
                disabled && styles.disabled,
              ]
        }
      >
        {({ pressed }) =>
          isGlass ? (
            <GlassSurface
              interactive={!disabled}
              tintColor={selected ? color : undefined}
              pressed={pressed}
              fallbackColor={selected ? color : undefined}
              style={[styles.center, circle]}
            >
              {glyph}
            </GlassSurface>
          ) : (
            glyph
          )
        }
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
  center: {
    alignItems: 'center',
    justifyContent: 'center',
  },
  button: {
    alignItems: 'center',
    justifyContent: 'center',
    borderWidth: 1,
    shadowOffset: { width: 0, height: 6 },
    shadowRadius: 14,
    elevation: 6,
  },
  disabled: {
    opacity: 0.35,
  },
  stopSquare: {
    borderRadius: 5,
  },
  label: typography.caption,
});
