import React, { FC } from 'react';
import { Pressable, StyleSheet, View } from 'react-native';
import Animated, {
  useAnimatedProps,
  withSpring,
} from 'react-native-reanimated';
import { RecordingState } from './types';

interface PauseButtonProps {
  state: RecordingState;
  onPress: () => void;
}

const PauseButton: FC<PauseButtonProps> = ({ state, onPress }) => (
  <View>
    <Pressable onPress={onPress} style={styles.pressable}>
      {({ pressed }) => <PauseButtonInner state={state} pressed={pressed} />}
    </Pressable>
  </View>
);

export default PauseButton;

const size = 24;
const innerSize = size * 0.3;

const PauseButtonInner: FC<{
  pressed: boolean;
  state: RecordingState;
}> = ({ pressed, state }) => {
  const leftViewStyle = useAnimatedProps(() => {
    return {
      transform: [
        {
          translateX: withSpring(pressed ? 6 : 0),
        },
      ],
    };
  });

  const rightViewStyle = useAnimatedProps(() => {
    return {
      transform: [
        {
          translateX: withSpring(pressed ? -6 : 0),
        },
      ],
    };
  });

  const containerStyle = useAnimatedProps(() => {
    return {
      transform: [
        {
          scale: withSpring(state === RecordingState.Recording ? 1 : 0),
        },
      ],
    };
  });

  return (
    <Animated.View style={[styles.container, containerStyle]}>
      <Animated.View style={[styles.bar, leftViewStyle]} />
      <Animated.View style={[styles.bar, rightViewStyle]} />
    </Animated.View>
  );
};

const styles = StyleSheet.create({
  pressable: {
    position: 'absolute',
    left: -size,
    top: -size / 2,
  },
  container: {
    width: size,
    height: size,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: innerSize - 2,
  },
  bar: {
    width: innerSize,
    height: size,
    backgroundColor: '#d4d4d4',
    borderRadius: 2,
  },
});
