import React, { useCallback, useEffect, useRef, useState, FC } from 'react';
import { ActivityIndicator } from 'react-native';
import {
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
  AudioBufferStreamSourceNode,
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
  const [playbackRate, setPlaybackRate] = useState(INITIAL_RATE * 1.5);
  const [detune, setDetune] = useState(INITIAL_DETUNE);

  const [audioBuffers, setAudioBuffers] = useState<AudioBuffer[]>([]);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const bufferStreamSourceRef = useRef<AudioBufferStreamSourceNode | null>(
    null
  );

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
      bufferStreamSourceRef.current?.stop(audioContextRef.current.currentTime);
      AudioManager.setLockScreenInfo({
        state: 'state_paused',
      });
    } else {
      if (!audioBuffers) {
        fetchAudioBuffer();
      }

      const now = audioContextRef.current.currentTime;

      bufferStreamSourceRef.current =
        audioContextRef.current.createBufferStreamSource();
      for (let i = 0; i < audioBuffers.length; i++) {
        bufferStreamSourceRef.current.enqueueAudioBuffer(audioBuffers[i]);
      }

      bufferStreamSourceRef.current.connect(
        audioContextRef.current.destination
      );
      bufferStreamSourceRef.current.playbackRate.value = playbackRate;
      bufferStreamSourceRef.current.start(now);
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

    if (!buffer) {
      setIsLoading(false);
      return;
    }

    const data = buffer.getChannelData(0);
    const buffers: AudioBuffer[] = [];

    for (let i = 0; i < 25; i++) {
      const buffer1 = audioContextRef.current!.createBuffer(
        buffer.numberOfChannels,
        buffer.sampleRate,
        buffer.sampleRate
      );

      const channelData = buffer1.getChannelData(0);

      for (let j = 0; j < buffer.sampleRate; j++) {
        channelData[j] = data[j + i * buffer.sampleRate];
      }

      // if (i === 1) {
      //   for (let j = 0; j < 1000; j++) {
      //     console.log(channelData[j]);
      //   }
      // }

      buffers.push(buffer1);
    }

    setAudioBuffers(buffers);

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

    const remotePlaySubscription = AudioManager.enableSystemEvent(
      'remotePlay',
      (event) => {
        console.log('remotePlay event:', event);
      }
    );

    const remotePauseSubscription = AudioManager.enableSystemEvent(
      'remotePause',
      (event) => {
        console.log('remotePause event:', event);
      }
    );

    const remoteChangePlaybackPositionSubscription =
      AudioManager.enableSystemEvent(
        'remoteChangePlaybackPosition',
        (event) => {
          console.log('remoteChangePlaybackPosition event:', event);
        }
      );

    const interruptionSubscription = AudioManager.enableSystemEvent(
      'interruption',
      (event) => {
        console.log('Interruption event:', event);
      }
    );

    AudioManager.observeAudioInterruptions(true);

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
        disabled={audioBuffers.length === 0}
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
