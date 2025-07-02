import React, { useCallback, useEffect, useState, FC } from 'react';
import { ActivityIndicator } from 'react-native';
import { AudioManager } from 'react-native-audio-api';
import { Container, Button } from '../../components';
import AudioPlayer from './AudioPlayer';

const URL =
  'https://software-mansion.github.io/react-native-audio-api/audio/voice/example-voice-01.mp3';

const AudioFile: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);

  const togglePlayPause = async () => {
    if (isPlaying) {
      await AudioPlayer.pause();
    } else {
      await AudioPlayer.play();

      AudioManager.observeAudioInterruptions(true);
    }

    setIsPlaying((prev) => !prev);
  };

  const fetchAudioBuffer = useCallback(async () => {
    setIsLoading(true);

    await AudioPlayer.loadBuffer(URL);

    setIsLoading(false);
  }, []);

  useEffect(() => {
    AudioManager.setLockScreenInfo({
      title: 'Audio file',
      artist: 'Software Mansion',
      album: 'Audio API',
      duration: 10,
    });

    AudioManager.enableRemoteCommand('remotePlay', true);
    AudioManager.enableRemoteCommand('remotePause', true);
    AudioManager.enableRemoteCommand('remoteSkipForward', true);
    AudioManager.enableRemoteCommand('remoteSkipBackward', true);
    AudioManager.observeAudioInterruptions(true);

    const remotePlaySubscription = AudioManager.addSystemEventListener(
      'remotePlay',
      () => {
        AudioPlayer.play();
      }
    );

    const remotePauseSubscription = AudioManager.addSystemEventListener(
      'remotePause',
      () => {
        AudioPlayer.pause();
      }
    );

    const remoteSkipForwardSubscription = AudioManager.addSystemEventListener(
      'remoteSkipForward',
      (event) => {
        AudioPlayer.seekBy(event.value);
      }
    );

    const remoteSkipBackwardSubscription = AudioManager.addSystemEventListener(
      'remoteSkipBackward',
      (event) => {
        AudioPlayer.seekBy(-event.value);
      }
    );

    const interruptionSubscription = AudioManager.addSystemEventListener(
      'interruption',
      async (event) => {
        if (event.type === 'began') {
          await AudioPlayer.pause();
          setIsPlaying(false);
        }
      }
    );

    fetchAudioBuffer();

    return () => {
      remotePlaySubscription?.remove();
      remotePauseSubscription?.remove();
      remoteSkipForwardSubscription?.remove();
      remoteSkipBackwardSubscription?.remove();
      interruptionSubscription?.remove();
      AudioManager.resetLockScreenInfo();
      AudioPlayer.reset();
    };
  }, [fetchAudioBuffer]);

  return (
    <Container centered>
      {isLoading && <ActivityIndicator color="#FFFFFF" />}
      <Button
        title={isPlaying ? 'Stop' : 'Play'}
        onPress={togglePlayPause}
        disabled={isLoading}
      />
    </Container>
  );
};

export default AudioFile;
