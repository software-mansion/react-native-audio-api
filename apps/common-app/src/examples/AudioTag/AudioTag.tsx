import React, { useRef } from 'react';
import { Button, View } from 'react-native';
import {
  Audio,
  AudioTagHandle,
} from 'react-native-audio-api/development/react';

import { Container } from '../../components';

// const DEMO_AUDIO_URL = 'https://filesamples.com/samples/audio/m4a/sample4.m4a';
const DEMO_AUDIO_URL = 'https://filesamples.com/samples/audio/mp3/sample4.mp3';

const AudioTag: React.FC = () => {
  const audioRef = useRef<AudioTagHandle>(null);

  // const handlePlay = () => {
  //   audioRef.current?.play();
  // };

  // const handlePause = () => {
  //   audioRef.current?.pause();
  // };

  // const handleSeekToTime = (time: number) => {
  //   console.log('handleSeekToTime', time);
  //   audioRef.current?.seekToTime(time);
  // };

  // const handleSetVolume = (volume: number) => {
  //   audioRef.current?.setVolume(volume);
  // };

  // const handleSetMuted = (muted: boolean) => {
  //   audioRef.current?.setMuted(muted);
  // };

  return (
    <Container disablePadding>
      <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
        <View style={{ width: '90%' }}>
          <Audio
            source={DEMO_AUDIO_URL}
            ref={audioRef}
            controls
            onLoadStart={() => console.log('onLoadStart')}
            onLoad={() => console.log('onLoad')}
            onError={(error) => console.log('onError', error)}
            onPositionChange={(seconds) =>
              console.log('onPositionChange', seconds)
            }
            onEnded={() => console.log('onEnded')}
            onPlay={() => console.log('onPlay')}
            onPause={() => console.log('onPause')}
            onVolumeChange={(volume) => console.log('onVolumeChange', volume)}
          />
        </View>
      </View>
    </Container>
  );
};

export default AudioTag;
