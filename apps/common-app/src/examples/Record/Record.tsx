import React, { useRef, FC } from 'react';
import { AudioManager, AudioRecorder } from 'react-native-audio-api';

import { Container, Button } from '../../components';

const Record: FC = () => {
  const recorderRef = useRef<AudioRecorder | null>(null);

  const onRecord = () => {
    AudioManager.setAudioSessionOptions({
      iosCategory: 'playAndRecord',
      iosMode: 'default',
      iosOptions: ['allowBluetoothA2DP', 'allowAirPlay'],
    });

    recorderRef.current = new AudioRecorder();

    recorderRef.current.start();

    setTimeout(() => {
      recorderRef.current?.stop();
      recorderRef.current = null;
    }, 1000);
  };
  return (
    <Container centered>
      <Button title="Record" onPress={onRecord} />
    </Container>
  );
};

export default Record;
