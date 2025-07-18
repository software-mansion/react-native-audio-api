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
    if (!recorderAdapterRef.current) {
      recorderAdapterRef.current = aCtxRef.current.createRecorderAdapter();
    }
    recorderAdapterRef.current.setRecorder(recorderRef.current);
    recorderAdapterRef.current.connect(aCtxRef.current.destination);
    console.log('Recorder adapter connected to audio context');
    
    if (aCtxRef.current.state === 'suspended') {
      console.log('Resuming audio context');
      aCtxRef.current.resume();
    }

  }, []);


  const start = () => {
    if (!aCtxRef.current || !recorderRef.current) {
      console.error('AudioContext or AudioRecorder is not initialized');
      return;
    }
    
    recorderRef.current.start();
    console.log('Recording started'); 
    console.log('Audio context state:', aCtxRef.current.state);
    if (aCtxRef.current.state === 'suspended') {
      console.log('Resuming audio context');
      aCtxRef.current.resume();
    }
  }

  /// This stops only the recording, not the audio context
  const stopRecorder = () => {
    if (!recorderRef.current) {
      console.error('AudioRecorder is not initialized');
      return;
    }
    recorderRef.current.stop();
    console.log('Recording stopped');
  }

  return (
    <Container centered>
      <Button title="Start Recording" onPress={start} />
      <Button title="Stop Recording" onPress={stopRecorder} />
    </Container>
  );
};

export default Record;
