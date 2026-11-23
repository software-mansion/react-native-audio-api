import { FC } from 'react';
import { Pressable, StyleSheet, Text } from 'react-native';

import { colors } from '../styles';
import GlassSurface from './GlassSurface';

interface ButtonProps {
  title: string;
  onPress: () => void;
  disabled?: boolean;
  /** The button grows past this to fit its title. */
  minWidth?: number;
  testID?: string;
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
  testID,
}) => {
  return (
    <Pressable
      testID={testID}
      disabled={disabled}
      onPress={onPress}
      accessibilityRole="button"
      accessibilityState={{ disabled }}
      style={styles.hitArea}
    >
      {({ pressed }) => (
        <GlassSurface
          interactive={!disabled}
          pressed={pressed}
          style={[styles.capsule, { minWidth }]}
        >
          <Text
            style={[styles.title, disabled && styles.titleDisabled]}
            numberOfLines={1}
          >
            {title}
          </Text>
        </GlassSurface>
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
