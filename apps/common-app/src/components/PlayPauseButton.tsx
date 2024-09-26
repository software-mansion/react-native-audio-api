import { FC } from 'react';
import { Text, TouchableOpacity, StyleSheet } from 'react-native';

import { colors, layout } from '../styles';

interface PlayPauseButtonProps {
  title: string;
  handlePlayPause: () => void;
}

const PlayPauseButton: FC<PlayPauseButtonProps> = (props) => {
  const { title, handlePlayPause } = props;

  return (
    <TouchableOpacity onPress={handlePlayPause} style={styles.button}>
      <Text style={styles.text}>{title}</Text>
    </TouchableOpacity>
  );
};

const styles = StyleSheet.create({
  button: {
    padding: layout.spacing,
    backgroundColor: colors.main,
    borderWidth: 1,
    borderColor: colors.black,
    borderRadius: layout.radius,
    width: 100,
  },
  text: {
    color: colors.black,
    textAlign: 'center',
  },
});

export default PlayPauseButton;
