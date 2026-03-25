import React, { useRef } from 'react';
import { View } from 'react-native';
import { Audio } from 'react-native-audio-api/development/react';

import { Container } from '../../components';
import { AudioContext } from 'react-native-audio-api';

const DEMO_AUDIO_URL =
  'https://filesamples.com/samples/audio/m4a/sample4.m4a';
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
