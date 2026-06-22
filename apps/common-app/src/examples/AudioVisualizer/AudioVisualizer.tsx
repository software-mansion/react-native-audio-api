import React, { useEffect, useRef, useState } from 'react';
import { ActivityIndicator, StyleSheet, View } from 'react-native';
import {
  AnalyserNode,
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
} from 'react-native-audio-api';

import { Button, Container } from '../../components';
import { layout } from '../../styles';
import FreqTimeChart from './FreqTimeChart';

const FFT_SIZE = 512;
const FREQUENCY_BIN_COUNT = FFT_SIZE / 2;

const URL =
  'https://software-mansion.github.io/react-native-audio-api/audio/music/example-music-02.mp3';

const AudioVisualizer: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);
  const [chartReady, setChartReady] = useState(false);

  const [startTime, setStartTime] = useState(0);
  const [offset, setOffset] = useState(0);

  const audioContextRef = useRef<AudioContext | null>(null);
  const analyserRef = useRef<AnalyserNode | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);

  const handlePlayPause = async () => {
    if (isPlaying) {
      const stopTime = audioContextRef.current!.currentTime;
      bufferSourceRef.current?.stop(stopTime);
      setOffset((prev) => prev + stopTime - startTime);
      setIsPlaying(false);
      return;
    }

    if (!audioContextRef.current || !analyserRef.current || !audioBuffer) {
      return;
    }

    await audioContextRef.current.resume();

    bufferSourceRef.current = audioContextRef.current.createBufferSource();
    bufferSourceRef.current.buffer = audioBuffer;
    bufferSourceRef.current.connect(analyserRef.current);

    const when = audioContextRef.current.currentTime;
    setStartTime(when);
    bufferSourceRef.current.start(when, offset);
    setIsPlaying(true);
  };

  const fetchAudioBuffer = async () => {
    if (!audioContextRef.current) {
      return;
    }

    setIsLoading(true);

    const buffer = await fetch(URL)
      .then((response) => response.arrayBuffer())
      .then((arrayBuffer) => audioContextRef.current!.decodeAudioData(arrayBuffer))
      .catch((error) => {
        console.error('Error decoding audio data source:', error);
        return null;
      });

    setAudioBuffer(buffer);
    setIsLoading(false);
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    if (!analyserRef.current) {
      analyserRef.current = new AnalyserNode(audioContextRef.current, {
        fftSize: FFT_SIZE,
        smoothingTimeConstant: 0.2,
      });
      analyserRef.current.connect(audioContextRef.current.destination);
    }

    fetchAudioBuffer();
    setChartReady(true);

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  return (
    <Container disablePadding>
      <View style={styles.main}>
        <View style={styles.chartArea}>
          {chartReady && analyserRef.current ? (
            <FreqTimeChart
              analyser={analyserRef.current}
              isPlaying={isPlaying}
              fftSize={FFT_SIZE}
              frequencyBinCount={FREQUENCY_BIN_COUNT}
            />
          ) : null}
        </View>
        <View style={styles.controls}>
          {isLoading && <ActivityIndicator color="#FFFFFF" />}
          <Button
            onPress={handlePlayPause}
            title={isPlaying ? 'Pause' : 'Play'}
            disabled={!audioBuffer}
          />
        </View>
      </View>
    </Container>
  );
};

const styles = StyleSheet.create({
  main: {
    flex: 1,
  },
  chartArea: {
    flex: 1,
    width: '100%',
    justifyContent: 'center',
    alignItems: 'stretch',
  },
  controls: {
    alignItems: 'center',
    justifyContent: 'center',
    paddingBottom: layout.spacing * 3,
    paddingTop: layout.spacing,
    minHeight: 96,
  },
});

export default AudioVisualizer;
