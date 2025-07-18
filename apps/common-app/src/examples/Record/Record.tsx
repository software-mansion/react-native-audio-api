import React, { useRef, FC, useEffect, act } from 'react';
import {
  AudioContext,
  AudioManager,
  AudioRecorder,
  RecorderAdapterNode
} from 'react-native-audio-api';

import { Container, Button } from '../../components';

const Record: FC = () => {
  const recorderRef = useRef<AudioRecorder | null>(null);
  const aCtxRef = useRef<AudioContext | null>(null);
  const recorderAdapterRef = useRef<RecorderAdapterNode | null>(null);


  useEffect(() => {
    AudioManager.setAudioSessionOptions({
      iosCategory: 'playAndRecord',
      iosMode: 'spokenAudio',
      iosOptions: ['allowBluetooth', 'defaultToSpeaker'],
    });

  }, []);


  const start = () => {
    if (!aCtxRef.current) {
      aCtxRef.current = new AudioContext({ sampleRate: 16000 });
    }
    if (!recorderRef.current) {
      recorderRef.current = new AudioRecorder({
        sampleRate: 16000,
        bufferLengthInSamples: 16000,
      });
    }

    /// Create the recorder adapter and connect it to the audio context
    recorderAdapterRef.current = aCtxRef.current.createRecorderAdapter();
    recorderAdapterRef.current.setRecorder(recorderRef.current);
    recorderAdapterRef.current.connect(aCtxRef.current.destination);
    recorderAdapterRef.current.start();

    // recorderRef.current.onAudioReady((event) => {
    //   const { buffer, numFrames, when } = event;
    //   console.log('Audio recorder buffer ready:', buffer.duration, numFrames, when);
    // });
    
    recorderRef.current.start();
    console.log('Recording started'); 
    console.log('Audio context state:', aCtxRef.current.state);
    if (aCtxRef.current.state === 'suspended') {
      console.log('Resuming audio context');
      aCtxRef.current.resume();
    }
  }

  return (
    <Container centered>
      <Button title="Start Recording" onPress={start} />
    </Container>
  );
};

export default Record;
