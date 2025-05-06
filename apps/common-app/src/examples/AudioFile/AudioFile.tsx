import React, { useCallback, useEffect, useRef, useState, FC } from 'react';
import { ActivityIndicator } from 'react-native';
import {
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
  AudioManager,
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

  const [offset, setOffset] = useState(0);
  const [playbackRate, setPlaybackRate] = useState(INITIAL_RATE);
  const [detune, setDetune] = useState(INITIAL_DETUNE);

  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);

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

    if (isPlaying) {
      bufferSourceRef.current?.stop(audioContextRef.current.currentTime);
      AudioManager.setLockScreenInfo({
        state: 'state_paused',
      });
    } else {
      if (!audioBuffer) {
        fetchAudioBuffer();
      }

      AudioManager.setLockScreenInfo({
        state: 'state_playing',
      });

      AudioManager.observeAudioInterruptions(true);

      bufferSourceRef.current = audioContextRef.current.createBufferSource({
        pitchCorrection: true,
      });
      bufferSourceRef.current.buffer = audioBuffer;
      bufferSourceRef.current.loop = true;
      bufferSourceRef.current.onended = (event) => {
        setOffset((_prev) => event.value || 0);
      };
      bufferSourceRef.current.loopStart = LOOP_START;
      bufferSourceRef.current.loopEnd = LOOP_END;
      bufferSourceRef.current.playbackRate.value = playbackRate;
      bufferSourceRef.current.detune.value = detune;
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

    AudioManager.setLockScreenInfo({
      title: 'Audio file',
      artist: 'Software Mansion',
      album: 'Audio API',
      duration: 10,
    });

    AudioManager.enableSystemEvent('remotePlay', (event) => {
      console.log('remotePlay event:', event);
    });

    AudioManager.enableSystemEvent('remotePause', (event) => {
      console.log('remotePause event:', event);
    });

    AudioManager.enableSystemEvent('remoteChangePlaybackPosition', (event) => {
      console.log('remoteChangePlaybackPosition event:', event);
    });

    AudioManager.enableSystemEvent('interruption', (event) => {
      console.log('Interruption event:', event);
    });
    AudioManager.observeAudioInterruptions(true);

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
