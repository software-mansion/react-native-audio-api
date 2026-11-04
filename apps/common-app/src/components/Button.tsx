import {
  isLiquidGlassSupported,
  LiquidGlassView,
} from '@callstack/liquid-glass';
import { FC } from 'react';
import { Pressable, StyleSheet, Text } from 'react-native';

import { colors } from '../styles';

interface ButtonProps {
  title: string;
  onPress: () => void;
  disabled?: boolean;
  /** The button grows past this to fit its title. */
  minWidth?: number;
}

const height = 44;

/**
 * A liquid glass capsule sized to its title. Before iOS 26 it falls back to a
 * plain surface capsule.
 */
const Button: FC<ButtonProps> = ({
  title,
  onPress,
  disabled = false,
  minWidth = 100,
}) => {
  return (
    <Pressable
      disabled={disabled}
      onPress={onPress}
      accessibilityRole="button"
      accessibilityState={{ disabled }}
      style={styles.hitArea}
    >
      {({ pressed }) => (
        <LiquidGlassView
          interactive={!disabled}
          colorScheme="dark"
          style={[
            styles.capsule,
            { minWidth },
            !isLiquidGlassSupported && styles.fallback,
            !isLiquidGlassSupported && pressed && styles.fallbackPressed,
          ]}
        >
          <Text
            style={[styles.title, disabled && styles.titleDisabled]}
            numberOfLines={1}
          >
            {title}
          </Text>
        </LiquidGlassView>
      )}
    </Pressable>
  );
};

const styles = StyleSheet.create({
  hitArea: {
    alignSelf: 'center',
  },
  capsule: {
    height,
    borderRadius: height / 2,
    paddingHorizontal: 20,
    alignItems: 'center',
    justifyContent: 'center',
  },
  fallback: {
    backgroundColor: colors.surface,
  },
  fallbackPressed: {
    backgroundColor: colors.surfacePressed,
  },
  title: {
    color: colors.textPrimary,
    fontSize: 15,
    fontWeight: '600',
  },
  titleDisabled: {
    color: colors.textMuted,
  },
});

export default Button;
