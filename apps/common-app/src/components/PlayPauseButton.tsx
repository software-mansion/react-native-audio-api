import { FC } from 'react';
import { Text, Pressable, StyleSheet } from 'react-native';

import { colors, layout } from '../styles';

interface PlayPauseButtonProps {
  title: string;
  handlePlayPause: () => void;
}

const PlayPauseButton: FC<PlayPauseButtonProps> = (props) => {
  const { title, handlePlayPause } = props;

  return (
    <Pressable
      onPress={handlePlayPause}
      style={({ pressed }) => [
        styles.button,
        { backgroundColor: pressed ? `${colors.main}88` : colors.main },
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
    width: 100,
  },
  text: {
    color: colors.white,
    textAlign: 'center',
  },
});

export default PlayPauseButton;
