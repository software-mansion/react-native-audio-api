import { FC } from 'react';
import { View, StyleSheet, Pressable } from 'react-native';

import { colors } from '../../styles';
import { KeyMap, Key, KeyName } from './utils';

interface KeyboardProps {
  keys: KeyMap;
  onPressIn: (key: KeyName) => void;
  onPressOut: (key: KeyName) => void;
}

interface KeyButtonProps {
  which: Key;
  index: number;
  onPressIn: (key: KeyName) => void;
  onPressOut: (key: KeyName) => void;
}

const KeyButton: FC<KeyButtonProps> = (props) => {
  const { which, index } = props;

  const onPressIn = () => {
    props.onPressIn(which.name);
  };

  const onPressOut = () => {
    props.onPressOut(which.name);
  };

  const isBlackKey = which.name.includes('#');
  const isFirst = index === 0;

  return (
    <Pressable
      onPressIn={onPressIn}
      onPressOut={onPressOut}
      style={({ pressed }) => [
        isBlackKey ? styles.blackKey : styles.key,
        !isFirst && !isBlackKey && styles.shiftKey,
        pressed && (isBlackKey ? styles.blackKeyPressed : styles.keyPressed),
      ]}
    />
  );
};

const Keyboard: FC<KeyboardProps> = (props) => {
  const { keys, onPressIn, onPressOut } = props;

  return (
    <View style={styles.container}>
      {Object.values(keys).map((key, index) => (
        <KeyButton
          which={key}
          index={index}
          key={key.name}
          onPressIn={onPressIn}
          onPressOut={onPressOut}
        />
      ))}
    </View>
  );
};

export default Keyboard;

const baseWidth = 50;

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
  },
  key: {
    width: baseWidth,
    height: baseWidth * 4.8,
    backgroundColor: colors.white,
    borderBottomLeftRadius: 4,
    borderBottomRightRadius: 4,
    borderWidth: 1,
    borderColor: colors.black,
  },
  shiftKey: { marginLeft: -1 },
  keyPressed: {
    backgroundColor: `#efefef`,
  },
  blackKey: {
    width: baseWidth * 0.6,
    height: baseWidth * 3.6,
    marginLeft: -baseWidth * 0.3,
    marginRight: -baseWidth * 0.3,
    zIndex: 2,
    backgroundColor: colors.black,
    borderBottomLeftRadius: 4,
    borderBottomRightRadius: 4,
  },
  blackKeyPressed: {
    backgroundColor: '#222',
  },
  text: {
    textAlign: 'center',
  },
});
