import React, { useState, useEffect, useRef } from 'react';
import {
  AudioContext,
  AnalyserNode,
  OscillatorNode,
} from 'react-native-audio-api';

import Layout from './Layout';
import FreqTimeChart from './FreqTimeChart';
import { Container, Button } from '../../components';

const AudioVisualizer: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [data, setData] = useState<number[]>([]);

  const audioContextRef = useRef<AudioContext | null>(null);
  const analyserRef = useRef<AnalyserNode | null>(null);
  const oscillatorRef = useRef<OscillatorNode | null>(null);

  const handlePlayPause = () => {
    if (isPlaying) {
      oscillatorRef.current?.stop(0);
    } else {
      if (!audioContextRef.current || !analyserRef.current) {
        return;
      }

      oscillatorRef.current = audioContextRef.current?.createOscillator();
      oscillatorRef.current.connect(analyserRef.current);
      oscillatorRef.current?.start(0);

      requestAnimationFrame(draw);
    }

    setIsPlaying((prev) => !prev);
  };

  const draw = () => {
    if (!analyserRef.current) {
      return;
    }

    const bufferLength = analyserRef.current.frequencyBinCount;
    const dataArray = new Array(bufferLength);
    // analyserRef.current.getByteTimeDomainData(dataArray);

    setData([1,1,1]);

    requestAnimationFrame(draw);
  }

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

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  return (
    <Container>
      <Layout.Box>
        <FreqTimeChart data={data} />
      </Layout.Box>
      <Button onPress={handlePlayPause} title={isPlaying ? 'Pause' : 'Play'} />
    </Container>
  );
};

export default AudioVisualizer;
