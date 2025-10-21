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
    const setup = async () => {
      try {
        await AudioManager.requestRecordingPermissions();
      } catch (err) {
        console.log(err);
        console.error('Recording permission denied', err);
        return;
      }
      recorderRef.current = new AudioRecorder({
        sampleRate: SAMPLE_RATE,
        bufferLengthInSamples: SAMPLE_RATE,
      });
    };

    setup();
    return () => {
      aCtxRef.current?.close();
      stopRecorder();
    };
  }, []);

  const setupRecording = () => {
    AudioManager.setAudioSessionOptions({
      iosCategory: 'playAndRecord',
      iosMode: 'spokenAudio',
      iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
    });
  };

  const stopRecorder = () => {
    if (recorderRef.current) {
      recorderRef.current.stop();
      console.log('Recording stopped');
      // advised, but not required
      AudioManager.setAudioSessionOptions({
        iosCategory: 'playback',
        iosMode: 'default',
      });
    } else {
      console.error('AudioRecorder is not initialized');
    }
  };

  const startEcho = () => {
    if (!recorderRef.current) {
      console.error('AudioContext or AudioRecorder is not initialized');
      return;
    }
    setupRecording();

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
    stopRecorder();
    aCtxRef.current = null;
    recorderAdapterRef.current = null;
  };

  const startRecordReplay = () => {
    if (!recorderRef.current) {
      console.error('AudioRecorder is not initialized');
      return;
    }
    setupRecording();
    audioBuffersRef.current = [];

    recorderRef.current.onAudioReady((event) => {
      const { buffer, numFrames } = event;

      console.log('Audio recorder buffer ready:', buffer.duration, numFrames);
      audioBuffersRef.current.push(buffer);
    });

    recorderRef.current.start();

    setTimeout(() => {
      stopRecorder();
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
      <Text style={{ color: colors.gray, fontSize: 18, textAlign: 'center' }}>
        Sample rate: {SAMPLE_RATE}
      </Text>
      <View style={{ alignItems: 'center', gap: 10, paddingTop: 20 }}>
        <Text style={{ color: colors.white, fontSize: 16 }}>Echo</Text>
        <Button title="Start Recording" onPress={startEcho} />
        <Button title="Stop Recording" onPress={stopEcho} />
      </View>
      <View style={{ alignItems: 'center', gap: 10, paddingTop: 40 }}>
        <Text style={{ color: colors.white, fontSize: 16 }}>
          Record & replay
        </Text>
        <Button title="Record for Replay" onPress={startRecordReplay} />
        <Button title="Replay" onPress={stopRecordReplay} />
      </View>
    </Container>
  );
};

export default Record;
