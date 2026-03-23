import { useAudioTagContext } from 'react-native-audio-api/development/react';
import { ActivityIndicator, Button, Text, View } from 'react-native';
import VolumeSlider from './VolumeSlider';
import { Spacer } from '../../components';

const AudioContent: React.FC = () => {
  const { isReady, play, pause, playbackState, setMuted, muted, seekToTime } = useAudioTagContext();

  return (
    <View>
      {!isReady ? (
        <ActivityIndicator color="#fff" />
      ) : (
        <View style={{ justifyContent: 'center', alignItems: 'center' }}>
          <Text style={{ color: '#fff' }}>{playbackState}</Text>
          <Spacer.Vertical size={12} />
          <Button onPress={() => {
            pause();
            setTimeout(() => {
              seekToTime(10);
            }, 50);
            setTimeout(() => {
              play();
            }, 50);
          }} title="Seek to 10" />
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
        </View>
      )}
    </View>
  );
};

export default AudioContent;
