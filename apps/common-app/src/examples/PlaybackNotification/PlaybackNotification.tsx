import React, { useState, useEffect, useRef } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView } from 'react-native';
import { PlaybackNotificationManager, AudioManager, AudioContext } from 'react-native-audio-api';
import type { PermissionStatus } from 'react-native-audio-api';
import { Container } from '../../components';
import { colors } from '../../styles';

export const PlaybackNotificationExample: React.FC = () => {
  const [permissionStatus, setPermissionStatus] = useState<PermissionStatus>('Undetermined');
  const [isRegistered, setIsRegistered] = useState(false);
  const [isShown, setIsShown] = useState(false);
  const [isPlaying, setIsPlaying] = useState(false);
  const audioContextRef = useRef<AudioContext | null>(null);

  useEffect(() => {
    // Create AudioContext in suspended state
    // Will be resumed when showing notification
    audioContextRef.current = new AudioContext();

    checkPermissions();

    // Add event listeners for notification actions
    const playListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationPlay',
      async () => {
        console.log('Notification: Play button pressed');
        setIsPlaying(true);
        // Update notification to reflect playing state
        await PlaybackNotificationManager.update({ state: 'playing' });
      }
    );

    const pauseListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationPause',
      async () => {
        console.log('Notification: Pause button pressed');
        setIsPlaying(false);
        // Update notification to reflect paused state
        await PlaybackNotificationManager.update({ state: 'paused' });
      }
    );

    const nextListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationNext',
      () => {
        console.log('Notification: Next button pressed');
      }
    );

    const previousListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationPrevious',
      () => {
        console.log('Notification: Previous button pressed');
      }
    );

    const skipForwardListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationSkipForward',
      (event) => {
        console.log(`Notification: Skip forward ${event.value}s`);
      }
    );

    const skipBackwardListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationSkipBackward',
      (event) => {
        console.log(`Notification: Skip backward ${event.value}s`);
      }
    );

    const dismissListener = PlaybackNotificationManager.addEventListener(
      'playbackNotificationDismissed',
      () => {
        console.log('Notification: Dismissed by user');
        setIsShown(false);
        setIsPlaying(false);
      }
    );

    return () => {
      playListener.remove();
      pauseListener.remove();
      nextListener.remove();
      previousListener.remove();
      skipForwardListener.remove();
      skipBackwardListener.remove();
      dismissListener.remove();

      // Cleanup AudioContext
      if (audioContextRef.current) {
        audioContextRef.current.close();
        audioContextRef.current = null;
      }
    };
  }, []);

  const checkPermissions = async () => {
    const status = await AudioManager.checkNotificationPermissions();
    setPermissionStatus(status);
  };

  const requestPermissions = async () => {
    const status = await AudioManager.requestNotificationPermissions();
    setPermissionStatus(status);
  };

  const handleRegister = async () => {
    try {
      await PlaybackNotificationManager.register();
      setIsRegistered(true);
      console.log('Playback notification registered');
    } catch (error) {
      console.error('Failed to register:', error);
    }
  };

  const handleShow = async () => {
    try {
      // Resume audio context to activate audio session on iOS
      if (audioContextRef.current?.state === 'suspended') {
        await audioContextRef.current.resume();
      }

      await PlaybackNotificationManager.show({
        title: 'My Audio Track',
        artist: 'Artist Name',
        album: 'Album Name',
        state: 'playing',
        artwork: 'https://images.pexels.com/photos/104827/cat-pet-animal-domestic-104827.jpeg?auto=compress&cs=tinysrgb&dpr=1&w=500',
        duration: 180,
        elapsedTime: 0,
      });
      setIsShown(true);
      setIsPlaying(true);
      console.log('Playback notification shown');
    } catch (error) {
      console.error('Failed to show:', error);
    }
  };

  const handlePlay = async () => {
    try {
      await PlaybackNotificationManager.update({
        state: 'playing',
        elapsedTime: 0,
      });
      setIsPlaying(true);
      console.log('Playing');
    } catch (error) {
      console.error('Failed to play:', error);
    }
  };

  const handlePause = async () => {
    try {
      await PlaybackNotificationManager.update({
        state: 'paused',
      });
      setIsPlaying(false);
      console.log('Paused');
    } catch (error) {
      console.error('Failed to pause:', error);
    }
  };

  const handleUpdateMetadata = async () => {
    try {
      await PlaybackNotificationManager.update({
        title: 'Updated Track',
        artist: 'New Artist',
        elapsedTime: 45,
      });
      console.log('Metadata updated');
    } catch (error) {
      console.error('Failed to update:', error);
    }
  };

  const handleHide = async () => {
    try {
      await PlaybackNotificationManager.hide();

      // Suspend audio context to deactivate audio session on iOS
      if (audioContextRef.current?.state === 'running') {
        await audioContextRef.current.suspend();
      }

      setIsShown(false);
      setIsPlaying(false);
      console.log('Playback notification hidden');
    } catch (error) {
      console.error('Failed to hide:', error);
    }
  };

  const handleUnregister = async () => {
    try {
      await PlaybackNotificationManager.unregister();
      setIsRegistered(false);
      setIsShown(false);
      setIsPlaying(false);
      console.log('Playback notification unregistered');
    } catch (error) {
      console.error('Failed to unregister:', error);
    }
  };

  const renderButton = (
    title: string,
    onPress: () => void,
    disabled: boolean = false
  ) => (
    <TouchableOpacity
      style={[styles.button, disabled && styles.buttonDisabled]}
      onPress={onPress}
      disabled={disabled}>
      <Text style={styles.buttonText}>{title}</Text>
    </TouchableOpacity>
  );

  return (
    <Container>
      <ScrollView style={styles.container}>
        <Text style={styles.title}>Playback Notification Test</Text>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Permission</Text>
          <Text style={styles.status}>Status: {permissionStatus}</Text>
          {renderButton('Request Permission', requestPermissions, permissionStatus === 'Granted')}
        </View>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Lifecycle</Text>
          {renderButton('Register', handleRegister, isRegistered)}
          {renderButton('Show', handleShow, !isRegistered || isShown)}
          {renderButton('Hide', handleHide, !isShown)}
          {renderButton('Unregister', handleUnregister, !isRegistered)}
        </View>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Playback Controls</Text>
          <Text style={styles.status}>State: {isPlaying ? 'Playing' : 'Paused'}</Text>
          {renderButton('Play', handlePlay, !isShown || isPlaying)}
          {renderButton('Pause', handlePause, !isShown || !isPlaying)}
          {renderButton('Update Metadata', handleUpdateMetadata, !isShown)}
        </View>
      </ScrollView>
    </Container>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    padding: 16,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    marginBottom: 20,
    color: colors.white,
  },
  section: {
    marginBottom: 24,
    paddingBottom: 16,
    borderBottomWidth: 1,
    borderBottomColor: colors.separator,
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: '600',
    marginBottom: 12,
    color: colors.white,
  },
  status: {
    fontSize: 14,
    color: colors.gray,
    marginBottom: 8,
  },
  button: {
    backgroundColor: colors.main,
    padding: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 8,
  },
  buttonDisabled: {
    backgroundColor: colors.border,
  },
  buttonText: {
    color: colors.white,
    fontSize: 16,
    fontWeight: '500',
  },
  info: {
    fontSize: 12,
    color: colors.gray,
    fontStyle: 'italic',
  },
});
