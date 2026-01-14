import React, { FC, useCallback, useEffect, useState } from 'react';
import { ActivityIndicator, Alert, StyleSheet, View } from 'react-native';
import {
  AudioManager,
  PlaybackNotificationManager,
} from 'react-native-audio-api';
import BackgroundTimer from 'react-native-background-timer';

import { Button, Container, Spacer } from '../../components';
import { colors } from '../../styles';
import AudioPlayer from './AudioPlayer';

// const remoteAsset =
//   'https://software-mansion.github.io/react-native-audio-api/audio/voice/example-voice-01.mp3';

import staticAsset from './voice-sample-landing.mp3';

const AudioFile: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [positionPercentage, setPositionPercentage] = useState(0);
  const [wasPlaying, setWasPlaying] = useState(false);

  const togglePlayPause = async () => {
    if (isPlaying) {
      await AudioPlayer.pause();
    } else {
      AudioPlayer.setOnPositionChanged((offset) => {
        setPositionPercentage(offset);
      });

      AudioManager.setAudioSessionOptions({
        iosCategory: 'playback',
        iosMode: 'default',
        iosOptions: [],
      });

      const success = await AudioManager.setAudioSessionActivity(true);

      if (!success) {
        Alert.alert(
          'Audio Session Error',
          'Failed to activate audio session for playback.'
        );
        return;
      }

      await AudioPlayer.play();

      AudioManager.observeAudioInterruptions(true);

      AudioManager.getDevicesInfo().then(console.log);
    }

    setIsPlaying((prev) => !prev);
  };

  const fetchAudioBuffer = useCallback(async () => {
    setIsLoading(true);

    await AudioPlayer.loadBuffer(staticAsset);

    setIsLoading(false);
  }, []);

  const setupNotification = async () => {
    try {
      await AudioManager.requestNotificationPermissions();
      const duration = AudioPlayer.getDuration();
      await PlaybackNotificationManager.show({
        title: 'Audio File',
        artist: 'Software Mansion',
        album: 'Audio API',
        duration: duration,
        state: 'paused',
        speed: 1.0,
        elapsedTime: 0,
      });
      await PlaybackNotificationManager.enableControl('skipBackward', true);
      await PlaybackNotificationManager.enableControl('next', true);
      await PlaybackNotificationManager.enableControl('skipForward', true);
      await PlaybackNotificationManager.enableControl('previous', true);
      await PlaybackNotificationManager.enableControl('pause', true);
      await PlaybackNotificationManager.enableControl('play', true);
      await PlaybackNotificationManager.enableControl('seekTo', true);

    } catch (error) {
      console.error('Failed to setup notification:', error);
    }
  };

  useEffect(() => {
    const setup = async () => {
      await fetchAudioBuffer();
      await setupNotification();
    };
    setup();
    return () => {
      AudioPlayer.reset();
      PlaybackNotificationManager.hide();
    };
  }, [fetchAudioBuffer]);

  useEffect(() => {
    AudioManager.observeAudioInterruptions(true);

    // Listen to notification control events
    const playListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationPlay',
      () => {
        AudioPlayer.play();
        setIsPlaying(true);
      }
    );

    const pauseListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationPause',
      () => {
        AudioPlayer.pause();
        setIsPlaying(false);
      }
    );

    const skipForwardListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationSkipForward',
      (event) => {
        AudioPlayer.seekBy(event.value);
      }
    );

    const skipBackwardListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationSkipBackward',
      (event) => {
        AudioPlayer.seekBy(-event.value);
      }
    );

    // Keep interruption handling through AudioManager
    const interruptionSubscription = AudioManager.addSystemEventListener(
      'interruption',
      async (event) => {
        if (event.type === 'began' && isPlaying) {
          await AudioPlayer.pause();
          setIsPlaying(false);
          setWasPlaying(true);
          return;
        }

        if (event.type === 'ended' && wasPlaying) {
          BackgroundTimer.setTimeout(async () => {
            AudioManager.setAudioSessionActivity(true);
            await AudioPlayer.play();
            setIsPlaying(true);
            setWasPlaying(false);
            console.log('Auto-resumed after transient interruption');
          }, 500);
        }
      }
    );

    const seekToListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationSeekTo',
      (event) => {
        AudioPlayer.seekBy(event.value - AudioPlayer.getElapsedTime());
      }
    );

    return () => {
      playListener.remove();
      pauseListener.remove();
      skipForwardListener.remove();
      skipBackwardListener.remove();
      seekToListener.remove();
      interruptionSubscription?.remove();
    };
  }, [isPlaying, wasPlaying]);

  return (
    <Container centered>
      {isLoading && <ActivityIndicator color="#FFFFFF" />}
      <Button
        title={isPlaying ? 'Stop' : 'Play'}
        onPress={togglePlayPause}
        disabled={isLoading}
      />
      <Spacer.Vertical size={20} />
      <View style={styles.progressContainer}>
        <View
          style={[
            styles.progressBar,
            { width: `${positionPercentage * 100}%` },
          ]}
        />
      </View>
    </Container>
  );
};

export default AudioFile;

const styles = StyleSheet.create({
  progressContainer: {
    width: '100%',
    height: 10,
    backgroundColor: '#333',
    borderRadius: 5,
    overflow: 'hidden',
    marginTop: 20,
  },
  progressBar: {
    height: '100%',
    backgroundColor: colors.main,
    borderRadius: 5,
  },
});
