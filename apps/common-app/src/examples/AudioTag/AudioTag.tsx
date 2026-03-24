import React, { useRef } from 'react';
import { View } from 'react-native';
import { Audio } from 'react-native-audio-api/development/react';

import { Container } from '../../components';
import { AudioContext } from 'react-native-audio-api';

const DEMO_AUDIO_URL =
  'https://software-mansion.github.io/react-native-audio-api/audio/music/example-music-02.mp3';
  // '/data/data/com.fabricexample/cache/audio.wav';

const AudioTag: React.FC = () => {
  const audioContext = useRef(new AudioContext());
  return (
    <Container disablePadding>
      <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
        <View style={{ width: '90%' }}>
          <Audio source={DEMO_AUDIO_URL} volume={1} controls context={audioContext.current} />
        </View>
      </View>
    </Container>
  );
};

export default AudioTag;
