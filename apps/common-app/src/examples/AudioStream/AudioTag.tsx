import React from 'react';
import { View } from 'react-native';
import { Audio } from 'react-native-audio-api/development/react';

import { Container } from '../../components';
import { layout } from '../../styles';
import AudioContent from './AudioContent';

const DEMO_AUDIO_URL =
  'https://filesamples.com/samples/audio/aac/sample4.aac';
  // '/data/data/com.fabricexample/cache/audio.wav';

const AudioTag: React.FC = () => {
  return (
    <Container disablePadding>
      <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
        <View style={{ marginTop: layout.spacing * 2 }}>
          <Audio source={DEMO_AUDIO_URL} volume={0.3} autoPlay>
            <AudioContent />
          </Audio>
        </View>
      </View>
    </Container>
  );
};

export default AudioTag;
