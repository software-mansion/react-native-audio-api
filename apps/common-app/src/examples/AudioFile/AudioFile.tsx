import React, { useCallback, useEffect, useRef, useState, FC } from 'react';
import { ActivityIndicator } from 'react-native';
import {
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
  AudioManager,
} from 'react-native-audio-api';
import { AudioPlayer } from '../../utils/AudioPlayer';

import { Container, Button, Spacer, Slider } from '../../components';

const URL =
  'https://software-mansion.github.io/react-native-audio-api/audio/voice/example-voice-01.mp3';

const INITIAL_RATE = 1;
const TRACK_LENGTH = 25;

const labelWidth = 80;

const AudioFile: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);

  const [offset, setOffset] = useState(0);
  const [playbackRate, setPlaybackRate] = useState(INITIAL_RATE);
  const [elapsedTime, setElapsedTime] = useState(0);

  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);
  const timerRef = useRef<NodeJS.Timeout | null>(null);

  const handlePlaybackRateChange = (newValue: number) => {
    setPlaybackRate(newValue);
    audioPlayer.setPlaybackRate(newValue);
  };

  const audioContext = useMemo(() => new AudioContext(), []);
  const audioPlayer = useMemo(() => new AudioPlayer(audioContext), []);

  useEffect(() => {
    return () => {
      audioPlayer.stop();
      audioContext.close();
    }
  }, []);

  useEffect(() => {
    timerRef.current = setTimeout(() => {
      if (isPlaying) {
        setElapsedTime((prev) => prev + 1);
      }
    }, 1000 / playbackRate);
    
    return () => clearTimeout(timerRef.current!);
  }, [isPlaying, playbackRate, elapsedTime]);

  useEffect(() => {
      AudioManager.setLockScreenInfo({
        elapsedTime: elapsedTime * 1000,
      });
      if (elapsedTime > TRACK_LENGTH) {
        setElapsedTime(0);
        setIsPlaying(false);
        audioPlayer.stop();
        AudioManager.setLockScreenInfo({
          state: 'state_paused',
        });
      }
  }, [elapsedTime]);


  const handlePress = useCallback(() => {
    if (isPlaying) {
      audioPlayer.stop();
      AudioManager.setLockScreenInfo({
        state: 'state_paused',
      });
    } else {
      if (!audioBuffer) {
        fetchAudioBuffer();
      }
      AudioManager.setLockScreenInfo({
        state: 'state_playing',
      })
      audioPlayer.buffer = audioBuffer!;
      audioPlayer.playbackRate = playbackRate;
      audioPlayer.play();
    }

    setIsPlaying((prev) => !prev);
  }, [isPlaying, audioBuffer, playbackRate]);

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
      duration: TRACK_LENGTH,
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
          audioPlayer.stop();
          if (isPlaying) {
            audioPlayer.play(
              event.value || 0
            );
          } else {
            audioPlayer.offset = event.value || 0;
          }
          setElapsedTime(event.value || 0);
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
    </Container>
  );
};

export default AudioFile;
