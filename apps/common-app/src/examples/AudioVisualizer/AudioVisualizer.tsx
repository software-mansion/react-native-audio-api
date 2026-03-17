import React, { useEffect, useRef, useState } from 'react';
import { ActivityIndicator, View } from 'react-native';
import {
  AnalyserNode,
  AudioContext,
  AudioFileSourceNode,
} from 'react-native-audio-api';

import { Button, Container } from '../../components';
import { layout } from '../../styles';
import FreqTimeChart from './FreqTimeChart';

const FFT_SIZE = 512;

const AUDIO_URL =
  'https://upload.wikimedia.org/wikipedia/commons/9/91/Dl1bajkiwisdr.ddns.net_2026-02-02T18_39_07Z_4625.00_usb.wav';

const AudioVisualizer: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [arrayBuffer, setArrayBuffer] = useState<ArrayBuffer | null>(null);

  const [times, setTimes] = useState<Uint8Array>(
    new Uint8Array(FFT_SIZE).fill(127)
  );
  const [freqs, setFreqs] = useState<Uint8Array>(
    new Uint8Array(FFT_SIZE / 2).fill(0)
  );

  const audioContextRef = useRef<AudioContext | null>(null);
  const analyserRef = useRef<AnalyserNode | null>(null);
  const fileSourceRef = useRef<AudioFileSourceNode | null>(null);
  const animFrameRef = useRef<number>(0);

  const draw = () => {
    if (!analyserRef.current) {
      return;
    }

    const timesArray = new Uint8Array(analyserRef.current.fftSize);
    analyserRef.current.getByteTimeDomainData(timesArray);
    setTimes(timesArray);

    const freqsArray = new Uint8Array(analyserRef.current.frequencyBinCount);
    analyserRef.current.getByteFrequencyData(freqsArray);
    setFreqs(freqsArray);

    // animFrameRef.current = requestAnimationFrame(draw);
  };

  const handlePlayPause = () => {
    if (!audioContextRef.current || !analyserRef.current || !arrayBuffer) {
      return;
    }

    if (isPlaying) {
      fileSourceRef.current?.stop(audioContextRef.current.currentTime);
      fileSourceRef.current = null;
      cancelAnimationFrame(animFrameRef.current);
    } else {
      fileSourceRef.current = audioContextRef.current.createAudioFileSource(
        arrayBuffer
      );
      fileSourceRef.current.connect(audioContextRef.current.destination);
      fileSourceRef.current.start(audioContextRef.current.currentTime);
      // animFrameRef.current = requestAnimationFrame(draw);
    }

    setIsPlaying((prev) => !prev);
  };

  useEffect(() => {
    audioContextRef.current = new AudioContext();
    analyserRef.current = new AnalyserNode(audioContextRef.current, {
      fftSize: FFT_SIZE,
      smoothingTimeConstant: 0.2,
    });
    // analyserRef.current.connect(audioContextRef.current.destination);

    setIsLoading(true);
    fetch(AUDIO_URL)
      .then((response) => response.arrayBuffer())
      .then((buffer) => {
        setArrayBuffer(buffer);
        setIsLoading(false);
      })
      .catch((error) => {
        console.error('Error fetching audio:', error);
        setIsLoading(false);
      });

    return () => {
      cancelAnimationFrame(animFrameRef.current);
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
        <View
          style={{
            justifyContent: 'center',
            flexDirection: 'row',
            marginTop: layout.spacing * 2,
          }}>
          <Button
            onPress={handlePlayPause}
            title={isPlaying ? 'Stop' : 'Play'}
            disabled={!arrayBuffer}
          />
        </View>
      </View>
    </Container>
  );
};

export default AudioVisualizer;
