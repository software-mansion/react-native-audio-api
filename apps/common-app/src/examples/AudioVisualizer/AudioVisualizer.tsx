import React, { useState, useEffect, useRef } from 'react';
import * as FileSystem from 'expo-file-system';
import {
  AudioContext,
  AnalyserNode,
  AudioBuffer,
  AudioBufferSourceNode,
} from 'react-native-audio-api';
import { ActivityIndicator, View, StyleSheet } from 'react-native';

import FreqTimeChart from './FreqTimeChart';
import { Container, Button } from '../../components';
import { layout } from '../../styles';

const FFT_SIZE = 512;

const URL =
  'https://software-mansion-labs.github.io/react-native-audio-api/audio/music/example-music-02.mp3';

const AudioVisualizer: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);

  const [times, setTimes] = useState<number[]>(new Array(FFT_SIZE).fill(127));
  const [freqs, setFreqs] = useState<number[]>(new Array(FFT_SIZE / 2).fill(0));

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

    const timesArrayLength = analyserRef.current.fftSize;
    const frequencyArrayLength = analyserRef.current.frequencyBinCount;

    const timesArray = new Array(timesArrayLength);
    analyserRef.current.getByteTimeDomainData(timesArray);
    setTimes(timesArray);

    const freqsArray = new Array(frequencyArrayLength);
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
      analyserRef.current.fftSize = FFT_SIZE;
      analyserRef.current.smoothingTimeConstant = 0.8;

      analyserRef.current.connect(audioContextRef.current.destination);
    }

    const fetchBuffer = async () => {
      setIsLoading(true);
      audioBufferRef.current = await FileSystem.downloadAsync(
        URL,
        FileSystem.documentDirectory + 'audio.mp3'
      ).then(({ uri }) => {
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
    <Container disablePadding>
      <View style={{ flex: 0.2 }} />
      <FreqTimeChart
        timeData={times}
        frequencyData={freqs}
        fftSize={analyserRef.current?.fftSize || FFT_SIZE}
        frequencyBinCount={
          analyserRef.current?.frequencyBinCount || FFT_SIZE / 2
        }
      />
      <View
        style={{ flex: 0.5, justifyContent: 'center', alignItems: 'center' }}>
        {isLoading && <ActivityIndicator color="#FFFFFF" />}
        <View style={styles.button}>
          <Button
            onPress={handlePlayPause}
            title={isPlaying ? 'Pause' : 'Play'}
            disabled={!audioBufferRef.current}
          />
        </View>
      </View>
    </Container>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  button: {
    justifyContent: 'center',
    flexDirection: 'row',
    marginTop: layout.spacing * 2,
  },
});

export default AudioVisualizer;
