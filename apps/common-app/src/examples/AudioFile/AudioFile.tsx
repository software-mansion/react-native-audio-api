import React, { useCallback, useEffect, useRef, useState, FC } from 'react';
import { ActivityIndicator } from 'react-native';
import {
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
} from 'react-native-audio-api';

import { Container, Button, Spacer, Slider } from '../../components';

const URL =
  'https://software-mansion.github.io/react-native-audio-api/audio/voice/example-voice-01.mp3';

const LOOP_START = 0;
const LOOP_END = 10;

const INITIAL_RATE = 1;
const INITIAL_DETUNE = 0;

const labelWidth = 80;

const AudioFile: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);

  const [playbackRate, setPlaybackRate] = useState(INITIAL_RATE);
  const [detune, setDetune] = useState(INITIAL_DETUNE);

  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const playbackRateRef = useRef<number>(INITIAL_RATE);
  const isPlayingRef = useRef<boolean>(false);

  useEffect(() => {
    isPlayingRef.current = isPlaying;
  }, [isPlaying]);

  useEffect(() => {
    playbackRateRef.current = playbackRate;
  }, [playbackRate]);

  const handlePlaybackRateChange = (newValue: number) => {
    setPlaybackRate(newValue);

    if (bufferSourceRef.current) {
      bufferSourceRef.current.playbackRate.value = newValue;
    }
  };

  const handleDetuneChange = (newValue: number) => {
    setDetune(newValue);

    if (bufferSourceRef.current) {
      bufferSourceRef.current.detune.value = newValue;
    }
  };

  const handlePress = () => {
    if (!audioContextRef.current) {
      return;
    }

    // if (isPlaying) {
    //   bufferSourceRef.current?.stop(audioContextRef.current.currentTime);
    // } else {
    //   if (!audioBuffer) {
    //     fetchAudioBuffer();
    //   }

    //   bufferSourceRef.current = audioContextRef.current.createBufferSource({
    //     pitchCorrection: true,
    //   });
    //   bufferSourceRef.current.buffer = audioBuffer;
    //   bufferSourceRef.current.playbackRate.value = playbackRate;
    //   bufferSourceRef.current.detune.value = detune;
    //   bufferSourceRef.current.connect(audioContextRef.current.destination);

    //   bufferSourceRef.current.start(audioContextRef.current.currentTime);
    // }

    setIsPlaying((prev) => !prev);
  };

  const fetchAudioBuffer = useCallback(async () => {
    setIsLoading(true);

    const buffer = await fetch(URL)
      .then((response) => response.arrayBuffer())
      .then((arrayBuffer) =>
        audioContextRef.current!.decodeAudioData(arrayBuffer)
      )
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

  useEffect(() => {
    function retryLoop() {
      if (!isPlayingRef.current || !audioBuffer) {
        return;
      }

      console.log('retryLoop');

      bufferSourceRef.current = audioContextRef.current!.createBufferSource({
        pitchCorrection: true,
      });

      bufferSourceRef.current.buffer = audioBuffer;
      bufferSourceRef.current.playbackRate.value = 2;
      bufferSourceRef.current.detune.value = 0;
      bufferSourceRef.current.connect(audioContextRef.current!.destination);

      bufferSourceRef.current.start(audioContextRef.current!.currentTime);

      setTimeout(retryLoop, (audioBuffer.duration * 1000) / 2 - 200);
    }

    if (isPlaying) {
      retryLoop();
    }
  }, [isPlaying, audioBuffer]);

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
        label="Playback rate"
        value={playbackRate}
        onValueChange={handlePlaybackRateChange}
        min={0.0}
        max={2.0}
        step={0.25}
        minLabelWidth={labelWidth}
      />
      <Spacer.Vertical size={20} />
      <Slider
        label="Detune"
        value={detune}
        onValueChange={handleDetuneChange}
        min={-1200}
        max={1200}
        step={100}
        minLabelWidth={labelWidth}
      />
    </Container>
  );
};

export default AudioFile;
