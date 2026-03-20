import { useContext } from 'react';
import { AudioComponentContext } from 'react-native-audio-api/development/react';
import { ActivityIndicator, Button, Text, View } from 'react-native';
import VolumeSlider from './VolumeSlider';
import { Spacer } from '../../components';

const AudioContent: React.FC = () => {
  const { isReady, play, pause, playbackState, setMuted, muted } = useContext(
    AudioComponentContext
  );

  return (
    <View>
      {!isReady ? (
        <ActivityIndicator color="#fff" />
      ) : (
        <>
          <Text style={{ color: '#fff' }}>{playbackState}</Text>
          <Spacer.Vertical size={12} />
          <Button
            onPress={() => setMuted(!muted)}
            title={muted ? 'Unmute' : 'Mute'}
          />
          <Spacer.Vertical size={20} />
          <Button onPress={() => {
            if (playbackState === 'playing') {
              pause();
            } else {
              play();
            }
          }} title={playbackState === 'playing' ? 'Pause' : 'Play'} />
          <Spacer.Vertical size={12} />
          <Spacer.Vertical size={50} />
          <VolumeSlider />
        </>
      )}
    </View>
  );
};

export default AudioContent;
