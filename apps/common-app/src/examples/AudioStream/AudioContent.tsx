import { useContext } from 'react';
import { AudioComponentContext } from 'react-native-audio-api/development/react';
import { ActivityIndicator, Button, View } from 'react-native';
import VolumeSlider from './VolumeSlider';
import { Spacer } from '../../components';

type Status = 'idle' | 'playing' | 'paused';

type IdleState = {
  status: 'idle';
}

type PlayingState = {
  status: 'playing';
  time: number;
}

type PausedState = {
  status: 'paused';
  time: number;
  pausedAt: number;
}

type State = IdleState | PlayingState | PausedState;

const AudioContent: React.FC = () => {
  const { isReady, play, setMuted, muted } = useContext(AudioComponentContext);

  // switch (state.status) {
  //   case 'idle':
  //     return <IdleState />;
  //   case 'playing':
  //     state.time;
  //   case 'paused':
  //     return <PausedState />;
  // }

  return (
    <View>
      {!isReady ?
        <ActivityIndicator color="#fff" /> :
        <>
        <Button onPress={() => setMuted(!muted)} title={muted ? 'Unmute' : 'Mute'}/>
        <Spacer.Vertical size={20} />
        <Button onPress={play} title="Play"/>
        <Spacer.Vertical size={50} />
        <VolumeSlider />
      </>
    }
    </View>
  );
};

export default AudioContent;
