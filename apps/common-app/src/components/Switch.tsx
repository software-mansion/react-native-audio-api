import React, { type FC } from 'react';
import { StyleSheet, Pressable } from 'react-native';
import Animated, {
  useAnimatedStyle,
  withSpring,
  withTiming,
} from 'react-native-reanimated';

import { colors } from '../styles';

interface SwitchProps {
  value: boolean;
  onValueChange: (value: boolean) => void;
}

const size = 24;

const Switch: FC<SwitchProps> = (props) => {
  const { value, onValueChange } = props;

  const onPress = () => {
    onValueChange(!value);
  };

  const containerStyle = useAnimatedStyle(() => ({
    backgroundColor: withTiming(value ? colors.main : colors.gray),
  }));

  const handleStyle = useAnimatedStyle(() => ({
    transform: [
      {
        translateX: withSpring(value ? size : 0),
      },
    ],
  }));

  return (
    <Pressable onPress={onPress}>
      <Animated.View style={[styles.container, containerStyle]}>
        <Animated.View
          collapsable={false}
          style={[styles.handle, handleStyle]}
        />
      </Animated.View>
    </Pressable>
  );
};

export default Switch;

const styles = StyleSheet.create({
  container: {
    height: size,
    width: 2 * size,
    alignItems: 'center',
    flexDirection: 'row',
    padding: size * 0.1,
    borderRadius: size / 2,
  },
  handle: {
    width: size * 0.8,
    height: size * 0.8,
    borderRadius: size / 2,
    backgroundColor: colors.white,
  },
});
