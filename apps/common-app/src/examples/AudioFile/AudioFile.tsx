import React, { FC, useEffect, useState } from 'react';
import { Alert, StyleSheet } from 'react-native';
import {
  AudioManager,
  PlaybackNotificationManager,
} from 'react-native-audio-api';
import BackgroundTimer from 'react-native-background-timer';

import { Container, ProgressBar, RoundButton } from '../../components';
import { layout } from '../../styles';
import { activatePlaybackSession, useAudioContext } from '../../utils/audio';
import AudioPlayer from './AudioPlayer';
import staticAsset from './voice-sample-landing.mp3';

async function showPlaybackNotification(duration: number) {
  await AudioManager.requestNotificationPermissions();
  await PlaybackNotificationManager.show({
    title: 'Audio File',
    artist: 'Software Mansion',
    artwork: 'https://wallpaperaccess.com/full/2658793.jpg',
    album: 'Audio API',
    androidSmallIcon: 'logo',
    duration,
    state: 'paused',
    speed: 1.0,
    elapsedTime: 0,
    skipInterval: 10, // seconds to skip on fast-forward/rewind; default: 15
  });
  await PlaybackNotificationManager.enableControl('skipBackward', true);
  await PlaybackNotificationManager.enableControl('nextTrack', true);
  await PlaybackNotificationManager.enableControl('skipForward', true);
  await PlaybackNotificationManager.enableControl('previousTrack', true);
  await PlaybackNotificationManager.enableControl('pause', true);
  await PlaybackNotificationManager.enableControl('play', true);
  await PlaybackNotificationManager.enableControl('seekTo', true);
}

const AudioFile: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(true);
  const [progress, setProgress] = useState(0);
  const [wasPlaying, setWasPlaying] = useState(false);

  const audioContext = useAudioContext();
  const [player] = useState(
    () =>
      new AudioPlayer(audioContext, {
        onPositionChanged: setProgress,
        onEnded: () => setIsPlaying(false),
      })
  );

  const togglePlayPause = async () => {
    if (isPlaying) {
      await player.pause();
      setIsPlaying(false);
      return;
    }

    try {
      await activatePlaybackSession();
    } catch (error) {
      console.error('Failed to activate audio session:', error);
      Alert.alert(
        'Audio Session Error',
        'Failed to activate audio session for playback.'
      );
      return;
    }

    await player.play();
    setIsPlaying(true);
  };

  useEffect(() => {
    let isCancelled = false;

    (async () => {
      await player.loadBuffer(staticAsset);
      if (isCancelled) {
        return;
      }
      setIsLoading(false);

      await showPlaybackNotification(player.getDuration());
      // Left the screen while the notification was being set up.
      if (isCancelled) {
        PlaybackNotificationManager.hide();
      }
    })().catch((error) => console.error('Failed to set up playback:', error));

    return () => {
      isCancelled = true;
      player.dispose();
      PlaybackNotificationManager.hide();
      AudioManager.setAudioSessionActivity(false);
    };
  }, [player]);

  useEffect(() => {
    AudioManager.observeAudioInterruptions(true);

    const playListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationPlay',
      () => {
        player.play();
        setIsPlaying(true);
      }
    );

    const pauseListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationPause',
      () => {
        player.pause();
        setIsPlaying(false);
      }
    );

    const skipForwardListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationSkipForward',
      (event) => player.seekBy(event.value)
    );

    const skipBackwardListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationSkipBackward',
      (event) => player.seekBy(-event.value)
    );

    const nextTrackListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationNextTrack',
      () => console.log('Next track event received from notification')
    );

    const previousTrackListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationPreviousTrack',
      () => console.log('Previous track event received from notification')
    );

    const seekToListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationSeekTo',
      (event) => player.seekBy(event.value - player.getElapsedTime())
    );

    const interruptionSubscription = AudioManager.addSystemEventListener(
      'interruption',
      async (event) => {
        if (event.type === 'began' && isPlaying) {
          await player.pause();
          setIsPlaying(false);
          setWasPlaying(true);
          return;
        }

        if (event.type === 'ended' && wasPlaying) {
          BackgroundTimer.setTimeout(async () => {
            player.setVolume(1.0);
            AudioManager.setAudioSessionActivity(true);
            await player.play();
            setIsPlaying(true);
            setWasPlaying(false);
          }, 500);
        }
      }
    );

    const duckListener = AudioManager.addSystemEventListener('duck', () => {
      player.setVolume(0.2);
    });

    return () => {
      playListener.remove();
      pauseListener.remove();
      skipForwardListener.remove();
      skipBackwardListener.remove();
      seekToListener.remove();
      interruptionSubscription?.remove();
      duckListener.remove();
      nextTrackListener.remove();
      previousTrackListener.remove();
    };
  }, [player, isPlaying, wasPlaying]);

  return (
    <Container centered style={styles.content}>
      <RoundButton
        icon={isPlaying ? 'pause' : 'play'}
        accessibilityLabel={isPlaying ? 'Pause' : 'Play'}
        onPress={togglePlayPause}
        loading={isLoading}
        disabled={isLoading}
      />
      <ProgressBar progress={progress} style={styles.progress} />
    </Container>
  );
};

export default AudioFile;

const styles = StyleSheet.create({
  content: {
    gap: layout.screenPadding * 2,
  },
  progress: {
    alignSelf: 'stretch',
  },
});
