import { useContext } from 'react';
import { AudioComponentContext } from 'react-native-audio-api/development/react';
import { Button } from 'react-native';

const AudioPlayerButton: React.FC = () => {
  const { play } = useContext(AudioComponentContext);

  return (
    <Button onPress={play} title="Play" />
  );
};

export default AudioPlayerButton;
