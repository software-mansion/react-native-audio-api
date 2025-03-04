import React, { useCallback, useEffect, useRef, useState, FC } from 'react';
import * as FileSystem from 'expo-file-system';
import { ActivityIndicator } from 'react-native';
import {
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
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

  const [offset, setOffset] = useState(0);
  const [playbackRate, setPlaybackRate] = useState(INITIAL_RATE);
  const [semitones, setSemitones] = useState(INITIAL_SEMITONES);

  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);

  const handlePlaybackRateChange = (newValue: number) => {
    setPlaybackRate(newValue);

    if (bufferSourceRef.current) {
      bufferSourceRef.current.playbackRate.value = newValue;
    }
  };

  const handleSemitonesChange = (newValue: number) => {
    setSemitones(newValue);

    if (bufferSourceRef.current) {
      bufferSourceRef.current.semitones.value = newValue;
    }
  };

  const handlePress = () => {
    if (!audioContextRef.current) {
      return;
    }

    if (isPlaying) {
      bufferSourceRef.current?.stop(audioContextRef.current.currentTime);
    } else {
      if (!audioBuffer) {
        fetchAudioBuffer();
      }

      bufferSourceRef.current = audioContextRef.current.createBufferSource();
      bufferSourceRef.current.buffer = audioBuffer;
      bufferSourceRef.current.loop = true;
      bufferSourceRef.current.onended = (stopTime: number) => {
        setOffset((_prev) => stopTime);
      };
      bufferSourceRef.current.loopStart = LOOP_START;
      bufferSourceRef.current.loopEnd = LOOP_END;
      bufferSourceRef.current.timeStretch = 'speech-music';
      bufferSourceRef.current.playbackRate.value = playbackRate;
      bufferSourceRef.current.semitones.value = semitones;
      bufferSourceRef.current.connect(audioContextRef.current.destination);

      bufferSourceRef.current.start(
        audioContextRef.current.currentTime,
        offset
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
        label="playbackRate"
        value={playbackRate}
        onValueChange={handlePlaybackRateChange}
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
