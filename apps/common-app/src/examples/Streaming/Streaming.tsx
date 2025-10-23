import React, { useRef, FC, useEffect, act } from 'react';
import {
  AudioContext,
  StreamerNode,
  GainNode
} from 'react-native-audio-api';

import { Container, Button } from '../../components';
import { View } from 'react-native';

const SAMPLE_RATE = 44100;

const Streaming: FC = () => {
  const streamerRef = useRef<StreamerNode | null>(null);
  const aCtxRef = useRef<AudioContext | null>(null);
  const gainRef = useRef<GainNode | null>(null);

  useEffect(() => {
    const actx = new AudioContext({ sampleRate: SAMPLE_RATE });
    aCtxRef.current = actx;
    gainRef.current = actx.createGain();
    return () => {
      aCtxRef.current?.close();
      streamerRef.current?.stop();
    }
  }, []);

  const startStreaming = () => {
    if (!aCtxRef.current || !gainRef.current) {
      console.error('AudioContext or gain or streamer is not initialized');
      return;
    }
    if (streamerRef.current) {
      console.error('StreamerNode is already initialized');
      return;
    }
    streamerRef.current = aCtxRef.current.createStreamer();

    streamerRef.current.initialize('https://liveradio.timesa.pl/2980-1.aac/playlist.m3u8');
    streamerRef.current.connect(gainRef.current);
    gainRef.current.connect(aCtxRef.current.destination);
    streamerRef.current.start(aCtxRef.current.currentTime);
  };

  const stopStreaming = () => {
    if (streamerRef.current) {
      streamerRef.current.stop(aCtxRef.current!.currentTime);
      streamerRef.current = null;
    } else {
      console.error('StreamerNode is not initialized');
    }
  };


  return (
    <Container style={{ gap: 40 }}>
      <View style={{ alignItems: 'center', justifyContent: 'center', gap: 5 }}>
        <Button title="Start streaming" onPress={startStreaming} />
        <Button title="Stop streaming" onPress={stopStreaming} />
        <Button title="Volume to 0.5" onPress={() => {
          if (gainRef.current) {
            gainRef.current.gain.value = 0.5;
          }
        }} />
        <Button title="Volume to 2.0" onPress={() => {
          if (gainRef.current) {
            gainRef.current.gain.value = 2.0;
          }
        }} />
      </View>
    </Container>
  );
};

export default Streaming;
