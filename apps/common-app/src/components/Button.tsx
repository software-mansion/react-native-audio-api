import { FC } from 'react';
import { Text, Pressable, StyleSheet } from 'react-native';

import { colors, layout } from '../styles';

interface ButtonProps {
  title: string;
  onPress: () => void;
  disabled?: boolean;
  width?: number;
}

const Button: FC<ButtonProps> = ({
  title,
  onPress,
  disabled = false,
  width = 100,
}) => {
  return (
    <Pressable
      disabled={disabled}
      onPress={onPress}
      style={({ pressed }) => [
        styles.button,
        {
          backgroundColor: pressed ? `${colors.main}88` : colors.main,
          opacity: disabled ? 0.5 : 1,
          width: width,
        },
      ]}
    >
      <Text style={styles.text}>{title}</Text>
    </Pressable>
  );
};

const styles = StyleSheet.create({
  button: {
    padding: layout.spacing,
    borderRadius: layout.radius,
  },
  text: {
    color: colors.white,
    textAlign: 'center',
  },
});

export default Button;
