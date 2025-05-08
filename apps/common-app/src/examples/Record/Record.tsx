import React, { useRef, FC, useEffect } from 'react';
import {
  AudioBuffer,
  AudioContext,
  AudioManager,
  AudioRecorder,
  AudioBufferSourceNode,
} from 'react-native-audio-api';

import { Container, Button } from '../../components';

const Record: FC = () => {
  const recorderRef = useRef<AudioRecorder | null>(null);
  const audioBuffersRef = useRef<AudioBuffer[]>([]);
  const sourcesRef = useRef<AudioBufferSourceNode[]>([]);
  const aCtxRef = useRef<AudioContext | null>(null);

  useEffect(() => {
    AudioManager.setAudioSessionOptions({
      iosCategory: 'playAndRecord',
      iosMode: 'spokenAudio',
      iosOptions: ['allowBluetooth', 'defaultToSpeaker'],
    });

    recorderRef.current = new AudioRecorder({
      sampleRate: 16000,
      bufferLengthInSamples: 16000,
    });
  }, []);

  const onReplay = () => {
    const aCtx = new AudioContext({ sampleRate: 16000 });
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
      source.onended = () => {
        console.log('Audio buffer source ended');
      };
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

  const onRecord = () => {
    if (!recorderRef.current) {
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
    }, 3000);
  };

  return (
    <Container centered>
      <Button title="Record" onPress={onRecord} />
      <Button title="Replay" onPress={onReplay} />
    </Container>
  );
};

export default Record;
