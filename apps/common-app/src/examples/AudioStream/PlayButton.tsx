import { useAudioTagContext } from 'react-native-audio-api/development/react';
import { Button } from 'react-native';

const AudioPlayerButton: React.FC = () => {
  const { play } = useAudioTagContext();

  return (
    <Button onPress={play} title="Play" />
  );
};

export default AudioPlayerButton;
