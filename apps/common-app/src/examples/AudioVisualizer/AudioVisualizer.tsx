import React, { useEffect, useRef, useState } from 'react';
import { ActivityIndicator, StyleSheet, View } from 'react-native';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
} from 'react-native-audio-api';
import { WorkletNode } from 'react-native-audio-worklets';
import { useSharedValue } from 'react-native-reanimated';

import { Button, Container } from '../../components';
import { layout } from '../../styles';
import FreqTimeChart from './FreqTimeChart';

const FFT_SIZE = 512;
const FREQUENCY_BIN_COUNT = FFT_SIZE / 2;

const URL =
  'https://software-mansion.github.io/react-native-audio-api/audio/music/example-music-02.mp3';

const ANALYSER_MIN_DB = -100;
const ANALYSER_MAX_DB = -30;

function linearMagnitudeToByte(linear: number) {
  'worklet';

  const db = linear > 0 ? 20 * Math.log10(linear) : ANALYSER_MIN_DB;
  const normalized = Math.max(
    0,
    Math.min(
      1,
      (db - ANALYSER_MIN_DB) / (ANALYSER_MAX_DB - ANALYSER_MIN_DB)
    )
  );

  return Math.round(normalized * 255);
}

const AudioVisualizer: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);
  const [visualizerReady, setVisualizerReady] = useState(false);

  const [startTime, setStartTime] = useState(0);
  const [offset, setOffset] = useState(0);

  const audioContextRef = useRef<AudioContext | null>(null);
  const timeWorkletRef = useRef<WorkletNode | null>(null);
  const frequencyWorkletRef = useRef<WorkletNode | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);

  const timeDataSV = useSharedValue(new Uint8Array(FFT_SIZE).fill(127));
  const frequencyDataSV = useSharedValue(
    new Uint8Array(FREQUENCY_BIN_COUNT).fill(0)
  );
  const timeDataTickSV = useSharedValue(0);
  const frequencyDataTickSV = useSharedValue(0);

  const handlePlayPause = async () => {
    if (isPlaying) {
      const stopTime = audioContextRef.current!.currentTime;
      audioContextRef.current?.suspend();
      bufferSourceRef.current?.stop(stopTime);
      setOffset((prev) => prev + stopTime - startTime);
      setIsPlaying(false);
      return;
    }

    const ctx = audioContextRef.current;
    const timeWorklet = timeWorkletRef.current;
    const frequencyWorklet = frequencyWorkletRef.current;

    if (!ctx || !timeWorklet || !frequencyWorklet || !audioBuffer) {
      return;
    }

    await ctx.resume();

    bufferSourceRef.current = ctx.createBufferSource();
    bufferSourceRef.current.buffer = audioBuffer;
    bufferSourceRef.current.connect(timeWorklet);

    const when = ctx.currentTime;
    setStartTime(when);
    bufferSourceRef.current.start(when, offset);
    setIsPlaying(true);
  };

  const fetchAudioBuffer = async () => {
    const ctx = audioContextRef.current;
    if (!ctx) {
      return;
    }

    setIsLoading(true);

    const buffer = await fetch(URL)
      .then((response) => response.arrayBuffer())
      .then((arrayBuffer) => ctx.decodeAudioData(arrayBuffer))
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

    timeWorkletRef.current = new WorkletNode(
      audioContextRef.current!,
      (audioData) => {
        'worklet';

        const snapshot = timeDataSV.value;

        for (let i = 0; i < audioData.length; i++) {
          const sample = Math.max(-1, Math.min(1, audioData[i]!));
          snapshot[i] = Math.round((sample + 1) * 127.5);
        }

        timeDataTickSV.value += 1;
      },
      { domain: 'time-domain', bufferLength: FFT_SIZE }
    );

    frequencyWorkletRef.current = new WorkletNode(
      audioContextRef.current!,
      (audioData) => {
        'worklet';

        const snapshot = frequencyDataSV.value;

        for (let i = 0; i < audioData.length; i++) {
          snapshot[i] = linearMagnitudeToByte(audioData[i]!);
        }

        frequencyDataTickSV.value += 1;
      },
      {
        domain: 'frequency-domain',
        bufferLength: FREQUENCY_BIN_COUNT,
      }
    );
    frequencyWorkletRef.current.smoothingTimeConstant = 0.2;

    timeWorkletRef.current.connect(frequencyWorkletRef.current);
    frequencyWorkletRef.current.connect(audioContextRef.current!.destination);

    fetchAudioBuffer();
    setVisualizerReady(true);

    return () => {
      timeWorkletRef.current?.disconnect();
      frequencyWorkletRef.current?.disconnect();
      timeWorkletRef.current = null;
      frequencyWorkletRef.current = null;
      audioContextRef.current!.close();
      audioContextRef.current = null;
    };
  }, [frequencyDataSV, frequencyDataTickSV, timeDataSV, timeDataTickSV]);

  return (
    <Container disablePadding>
      <View style={styles.main}>
        <View style={styles.chartArea}>
          {visualizerReady ? (
            <FreqTimeChart
              timeDataSV={timeDataSV}
              timeDataTickSV={timeDataTickSV}
              frequencyDataSV={frequencyDataSV}
              frequencyDataTickSV={frequencyDataTickSV}
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
