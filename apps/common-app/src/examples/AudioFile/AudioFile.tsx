import React, { useCallback, useEffect, useRef, useState, FC } from 'react';
import * as FileSystem from 'expo-file-system';
import { ActivityIndicator } from 'react-native';
import {
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
  StretcherNode,
} from 'react-native-audio-api';

import { Container, Button, Spacer, Slider } from '../../components';

const URL =
  'https://software-mansion-labs.github.io/react-native-audio-api/audio/music/example-music-02.mp3';

const LOOP_START = 0;
const LOOP_END = 10;

const INITIAL_RATE = 1;
const INITIAL_SEMITONES = 0;

const labelWidth = 80;

const AudioFile: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);

  const [startTime, setStartTime] = useState(0);
  const [offset, setOffset] = useState(0);
  const [rate, setRate] = useState(INITIAL_RATE);
  const [semitones, setSemitones] = useState(INITIAL_SEMITONES);

  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const stretcherRef = useRef<StretcherNode | null>(null);

  const handleRateChange = (newValue: number) => {
    setRate(newValue);

    if (stretcherRef.current) {
      stretcherRef.current.rate.value = newValue;
    }
  };

  const handleSemitonesChange = (newValue: number) => {
    setSemitones(newValue);

    if (stretcherRef.current) {
      stretcherRef.current.semitones.value = newValue;
    }
  };

  const handlePress = () => {
    if (!audioContextRef.current || !stretcherRef.current) {
      return;
    }

    if (isPlaying) {
      const stopTime = audioContextRef.current.currentTime;
      bufferSourceRef.current?.stop(stopTime);
      setOffset((prev) => prev + stopTime - startTime);
    } else {
      if (!audioBuffer) {
        fetchAudioBuffer();
      }

      bufferSourceRef.current = audioContextRef.current.createBufferSource();
      bufferSourceRef.current.buffer = audioBuffer;
      bufferSourceRef.current.loop = true;
      bufferSourceRef.current.loopStart = LOOP_START;
      bufferSourceRef.current.loopEnd = LOOP_END;
      bufferSourceRef.current.connect(stretcherRef.current);

      setStartTime(audioContextRef.current.currentTime);
      bufferSourceRef.current.start(
        startTime,
        offset * stretcherRef.current.rate.value
      );
    }

    setIsPlaying((prev) => !prev);
  };

  const fetchAudioBuffer = useCallback(async () => {
    setIsLoading(true);
    const buffer = await FileSystem.downloadAsync(
      URL,
      FileSystem.documentDirectory + 'audio.mp3'
    )
      .then(({ uri }) => {
        return audioContextRef.current!.decodeAudioDataSource(uri);
      })
      .catch((error) => {
        console.error('Error decoding audio data source:', error);
        return null;
      });

    setAudioBuffer(buffer);

    setIsLoading(false);
  }, []);

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    if (!stretcherRef.current) {
      stretcherRef.current = audioContextRef.current.createStretcher();
      stretcherRef.current.connect(audioContextRef.current.destination);
    }

    fetchAudioBuffer();

    return () => {
      audioContextRef.current?.close();
    };
  }, [fetchAudioBuffer]);

  return (
    <Container centered>
      {isLoading && <ActivityIndicator color="#FFFFFF" />}
      <Button
        title={isPlaying ? 'Stop' : 'Play'}
        onPress={handlePress}
        disabled={!audioBuffer}
      />
      <Spacer.Vertical size={49} />
      <Slider
        label="Rate"
        value={rate}
        onValueChange={handleRateChange}
        min={0.0}
        max={3.0}
        step={0.25}
        minLabelWidth={labelWidth}
      />
      <Spacer.Vertical size={20} />
      <Slider
        label="Semitones"
        value={semitones}
        onValueChange={handleSemitonesChange}
        min={-12}
        max={12}
        step={1}
        minLabelWidth={labelWidth}
      />
    </Container>
  );
};

export default AudioFile;
