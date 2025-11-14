import React, { useCallback, useEffect, useState, FC } from 'react';
import { ActivityIndicator, View, StyleSheet } from 'react-native';
import { AudioManager, PlaybackNotificationManager } from 'react-native-audio-api';
import { Container, Button, Spacer } from '../../components';
import AudioPlayer from './AudioPlayer';
import { colors } from '../../styles';
import BackgroundTimer from 'react-native-background-timer';

const URL =
  'https://software-mansion.github.io/react-native-audio-api/audio/voice/example-voice-01.mp3';

const AudioFile: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [positionPercentage, setPositionPercentage] = useState(0);
  const [shouldResume, setShouldResume] = useState(false);

  const togglePlayPause = async () => {
    if (isPlaying) {
      await AudioPlayer.pause();
    } else {
      AudioPlayer.setOnPositionChanged((offset) => {
        setPositionPercentage(offset);
      });

      await AudioPlayer.play();

      AudioManager.observeAudioInterruptions(true);

      AudioManager.getDevicesInfo().then(console.log);
    }

    setIsPlaying((prev) => !prev);
  };

  const fetchAudioBuffer = useCallback(async () => {
    setIsLoading(true);

    await AudioPlayer.loadBuffer(URL);

    setIsLoading(false);
  }, []);

  useEffect(() => {
    // Register notification first
    const setupNotification = async () => {
      try {
        await PlaybackNotificationManager.register();

        // Load audio buffer first
        await fetchAudioBuffer();

        // Show notification with correct duration after buffer is loaded
        const duration = AudioPlayer.getDuration();
        await PlaybackNotificationManager.show({
          title: 'Audio file',
          artist: 'Software Mansion',
          album: 'Audio API',
          duration: duration,
          state: 'paused',
          speed: 1.0,
          elapsedTime: 0,
        });
      } catch (error) {
        console.error('Failed to setup notification:', error);
      }
    };

    setupNotification();

    AudioManager.observeAudioInterruptions(true);
    AudioManager.activelyReclaimSession(true);

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
        if (event.type === 'began') {
          // Store whether we were playing before interruption
          setShouldResume(isPlaying && event.isTransient);

          if (isPlaying) {
            await AudioPlayer.pause();
            setIsPlaying(false);
          }
        } else if (event.type === 'ended') {

          if (shouldResume) {
            BackgroundTimer.setTimeout(async () => {
              AudioManager.setAudioSessionActivity(true);
              await AudioPlayer.play();
              setIsPlaying(true);
              console.log('Auto-resumed after transient interruption');
            }, 500);
          }

          // Reset the flag
          setShouldResume(false);
        }
      }
    );

    return () => {
      playListener.remove();
      pauseListener.remove();
      skipForwardListener.remove();
      skipBackwardListener.remove();
      interruptionSubscription?.remove();
      PlaybackNotificationManager.unregister();
      AudioPlayer.reset();
      console.log('Cleanup AudioFile component');
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
