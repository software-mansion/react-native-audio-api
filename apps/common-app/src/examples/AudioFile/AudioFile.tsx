import React, { useCallback, useEffect, useRef, useState, FC } from 'react';
import { ActivityIndicator } from 'react-native';
import {
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
  AudioManager,
} from 'react-native-audio-api';

import { Container, Button, Spacer, Slider } from '../../components';
import { EventTypeWithValue } from 'react-native-audio-api/lib/typescript/events/types';

const URL =
  'https://software-mansion.github.io/react-native-audio-api/audio/voice/example-voice-01.mp3';

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

  const handlePress = async () => {
    if (!audioContextRef.current) {
      return;
    }

    if (isPlaying) {
      bufferSourceRef.current?.stop(audioContextRef.current.currentTime);
      AudioManager.setLockScreenInfo({
        state: 'state_paused',
      });

      setTimeout(async () => {
        await audioContextRef.current?.suspend();
      }, 5);
    } else {
      if (!audioBuffer) {
        fetchAudioBuffer();
      }

      await audioContextRef.current.resume();

      AudioManager.setLockScreenInfo({
        state: 'state_playing',
      });

      AudioManager.observeAudioInterruptions(true);

      bufferSourceRef.current = audioContextRef.current.createBufferSource({
        pitchCorrection: true,
      });
      bufferSourceRef.current.buffer = audioBuffer;
      bufferSourceRef.current.onended = (event) => {
        setOffset((_prev) => event.value || 0);
      };
      bufferSourceRef.current.playbackRate.value = playbackRate;
      bufferSourceRef.current.detune.value = detune;
      bufferSourceRef.current.connect(audioContextRef.current.destination);
      bufferSourceRef.current.onPositionChanged = (event: EventTypeWithValue) => {
        console.log('onPositionChanged event:', event);
      };
      // bufferSourceRef.current.onPositionChangedInterval = 200;
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
      audioContextRef.current = new AudioContext({ initSuspended: true });
    }

    AudioManager.setLockScreenInfo({
      title: 'Audio file',
      artist: 'Software Mansion',
      album: 'Audio API',
      duration: 10,
    });

    AudioManager.enableRemoteCommand('remotePlay', true);
    AudioManager.enableRemoteCommand('remotePause', true);
    AudioManager.enableRemoteCommand('remoteChangePlaybackPosition', true);
    AudioManager.observeAudioInterruptions(true);

    const remotePlaySubscription = AudioManager.addSystemEventListener(
      'remotePlay',
      (event) => {
        console.log('remotePlay event:', event);
      }
    );

    const remotePauseSubscription = AudioManager.addSystemEventListener(
      'remotePause',
      (event) => {
        console.log('remotePause event:', event);
      }
    );

    const remoteChangePlaybackPositionSubscription =
      AudioManager.addSystemEventListener(
        'remoteChangePlaybackPosition',
        (event) => {
          console.log('remoteChangePlaybackPosition event:', event);
        }
      );

    const interruptionSubscription = AudioManager.addSystemEventListener(
      'interruption',
      (event) => {
        console.log('Interruption event:', event);
      }
    );

    fetchAudioBuffer();

    return () => {
      remotePlaySubscription?.remove();
      remotePauseSubscription?.remove();
      remoteChangePlaybackPositionSubscription?.remove();
      interruptionSubscription?.remove();
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
