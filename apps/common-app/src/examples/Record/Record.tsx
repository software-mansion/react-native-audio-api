import React, { useRef, FC, useEffect } from 'react';
import {
  AudioContext,
  AudioManager,
  AudioRecorder,
  RecorderAdapterNode,
  AudioBufferSourceNode,
  AudioBuffer,
} from 'react-native-audio-api';

import { Container, Button } from '../../components';
import { View, Text } from 'react-native';
import { colors } from '../../styles';

const SAMPLE_RATE = 16000;

const Record: FC = () => {
  const recorderRef = useRef<AudioRecorder | null>(null);
  const aCtxRef = useRef<AudioContext | null>(null);
  const recorderAdapterRef = useRef<RecorderAdapterNode | null>(null);
  const audioBuffersRef = useRef<AudioBuffer[]>([]);
  const sourcesRef = useRef<AudioBufferSourceNode[]>([]);

  useEffect(() => {
    AudioManager.setAudioSessionOptions({
      iosCategory: 'playAndRecord',
      iosMode: 'spokenAudio',
      iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
    });

    AudioManager.requestRecordingPermissions();

    recorderRef.current = new AudioRecorder({
      sampleRate: SAMPLE_RATE,
      bufferLengthInSamples: SAMPLE_RATE,
    });
  }, []);

  const startEcho = () => {
    if (!recorderRef.current) {
      console.error('AudioContext or AudioRecorder is not initialized');
      return;
    }

    aCtxRef.current = new AudioContext({ sampleRate: SAMPLE_RATE });
    recorderAdapterRef.current = aCtxRef.current.createRecorderAdapter();
    recorderAdapterRef.current.connect(aCtxRef.current.destination);
    recorderRef.current.connect(recorderAdapterRef.current);

    recorderRef.current.start();
    console.log('Recording started');
    console.log('Audio context state:', aCtxRef.current.state);
    if (aCtxRef.current.state === 'suspended') {
      console.log('Resuming audio context');
      aCtxRef.current.resume();
    }
  };

  /// This stops only the recording, not the audio context
  const stopEcho = () => {
    if (!recorderRef.current) {
      console.error('AudioRecorder is not initialized');
      return;
    }
    recorderRef.current.stop();
    aCtxRef.current = null;
    recorderAdapterRef.current = null;
    console.log('Recording stopped');
  };

  const startRecordReplay = () => {
    if (!recorderRef.current) {
      console.error('AudioRecorder is not initialized');
      return;
    }

    recorderRef.current.onAudioReady((event) => {
      const { buffer, numFrames, when } = event;

      console.log(
        'Audio recorder buffer ready:',
        buffer.duration,
        numFrames,
        when
      );
      audioBuffersRef.current.push(buffer);
    });

    recorderRef.current.start();

    setTimeout(() => {
      recorderRef.current?.stop();
      console.log('Recording stopped');
    }, 5000);
  };

  const stopRecordReplay = () => {
    const aCtx = new AudioContext({ sampleRate: SAMPLE_RATE });
    aCtxRef.current = aCtx;

    if (aCtx.state === 'suspended') {
      aCtx.resume();
    }

    const tNow = aCtx.currentTime;
    let nextStartAt = tNow + 1;
    const buffers = audioBuffersRef.current;

    console.log(tNow, nextStartAt, buffers.length);

    for (let i = 0; i < buffers.length; i++) {
      const source = aCtx.createBufferSource();
      source.buffer = buffers[i];

      source.connect(aCtx.destination);
      sourcesRef.current.push(source);

      source.start(nextStartAt);
      nextStartAt += buffers[i].duration;
    }

    setTimeout(
      () => {
        console.log('clearing data');
        audioBuffersRef.current = [];
        sourcesRef.current = [];
      },
      (nextStartAt - tNow) * 1000
    );
  };

  return (
    <Container style={{ gap: 40 }}>
      <Text style={{ color: colors.white, fontSize: 24, textAlign: 'center' }}>
        Sample rate: {SAMPLE_RATE}
      </Text>
      <View style={{ alignItems: 'center', justifyContent: 'center', gap: 5 }}>
        <Text style={{ color: colors.white, fontSize: 24 }}>Echo example</Text>
        <Button title="Start Recording" onPress={startEcho} />
        <Button title="Stop Recording" onPress={stopEcho} />
      </View>
      <View style={{ alignItems: 'center', justifyContent: 'center', gap: 5 }}>
        <Text style={{ color: colors.white, fontSize: 24 }}>
          Record & replay example
        </Text>
        <Button title="Record for Replay" onPress={startRecordReplay} />
        <Button title="Replay" onPress={stopRecordReplay} />
      </View>
    </Container>
  );
};

export default Record;
