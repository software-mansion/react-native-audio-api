import React from 'react';
import { View } from 'react-native';
import { Audio } from 'react-native-audio-api/development/react';

import { Container } from '../../components';
import AudioContent from './AudioContent';

const DEMO_AUDIO_URL =
  'https://filesampleshub.com/download/audio/aac/sample1.AAC';
  // '/data/data/com.fabricexample/cache/audio.wav';

const AudioTag: React.FC = () => {
  return (
    <Container disablePadding>
      <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
        <View style={{ width: '90%' }}>
          <Audio source={DEMO_AUDIO_URL} volume={0.7} controls/>
            {/* <AudioContent /> */}
          {/* </Audio> */}
        </View>
      </View>
    </Container>
  );
};

export default AudioTag;
