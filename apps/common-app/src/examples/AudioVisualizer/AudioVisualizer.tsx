import React, { useState, useEffect, useRef } from 'react';
import * as FileSystem from 'expo-file-system';
import {
  AudioContext,
  AnalyserNode,
  AudioBuffer,
  AudioBufferSourceNode,
} from 'react-native-audio-api';
import { ActivityIndicator } from 'react-native';

import Layout from './Layout';
import FreqTimeChart from './FreqTimeChart';
import { Container, Button } from '../../components';

const URL =
  'https://maciejmakowski2003.github.io/audio-samples/sounds/chrono.mp3';

const AudioVisualizer: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);

  const [times, setTimes] = useState<number[]>([]);
  const [freqs, setFreqs] = useState<number[]>([]);

  const audioContextRef = useRef<AudioContext | null>(null);
  const analyserRef = useRef<AnalyserNode | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);

  const handlePlayPause = () => {
    if (isPlaying) {
      bufferSourceRef.current?.stop();
    } else {
      if (!audioContextRef.current || !analyserRef.current) {
        return;
      }

      bufferSourceRef.current = audioContextRef.current.createBufferSource();
      bufferSourceRef.current.buffer = audioBufferRef.current;
      bufferSourceRef.current.connect(analyserRef.current);

      bufferSourceRef.current.start();

      requestAnimationFrame(draw);
    }

    setIsPlaying((prev) => !prev);
  };

  const draw = () => {
    if (!analyserRef.current) {
      return;
    }

    const bufferLength = analyserRef.current.frequencyBinCount;

    const timesArray = new Array(bufferLength);
    analyserRef.current.getByteTimeDomainData(timesArray);
    setTimes(timesArray);

    const freqsArray = new Array(bufferLength);
    analyserRef.current.getByteFrequencyData(freqsArray);
    setFreqs(freqsArray);

    requestAnimationFrame(draw);
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    if (!analyserRef.current) {
      analyserRef.current = audioContextRef.current.createAnalyser();
      analyserRef.current.fftSize = 2048;
      analyserRef.current.smoothingTimeConstant = 0.85;
      analyserRef.current.minDecibels = -90;
      analyserRef.current.maxDecibels = -10;

      analyserRef.current.connect(audioContextRef.current.destination);
    }

    const fetchBuffer = async () => {
      setIsLoading(true);
      audioBufferRef.current = await FileSystem.downloadAsync(
        URL,
        FileSystem.documentDirectory + 'audio.mp3'
      )
        .then(({ uri }) => {
          return uri.replace('file://', '');
        })
        .then((uri) => {
          return audioContextRef.current!.decodeAudioDataSource(uri);
        });
      setIsLoading(false);
    };

    fetchBuffer();

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  return (
    <Container>
      <Layout.Row>
        <Layout.Box>
          <FreqTimeChart data={times} />
        </Layout.Box>
      </Layout.Row>
      <Layout.LineHorizontal />
      <Layout.Row>
        <Layout.Box>
          <FreqTimeChart data={freqs} />
        </Layout.Box>
      </Layout.Row>
      {isLoading && <ActivityIndicator color="#FFFFFF" />}
      <Button
        onPress={handlePlayPause}
        title={isPlaying ? 'Pause' : 'Play'}
        disabled={!audioBufferRef.current}
      />
    </Container>
  );
};

export default AudioVisualizer;
