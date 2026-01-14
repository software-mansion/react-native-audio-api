import React, { FC, useCallback, useEffect, useState } from 'react';
import { AudioManager, RecordingNotificationManager } from 'react-native-audio-api';

import { Alert, StyleSheet, View } from 'react-native';
import { Container } from '../../components';

import { audioRecorder as Recorder } from '../../singletons';
import ControlPanel from './ControlPanel';
import RecordingTime from './RecordingTime';
import RecordingVisualization from './RecordingVisualization';
import Status from './Status';
import { RecordingState } from './types';
import { AudioEventSubscription } from 'react-native-audio-api/lib/typescript/events';

AudioManager.setAudioSessionOptions({
  iosCategory: 'playAndRecord',
  iosMode: 'default',
  iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
});

const Record: FC = () => {
  const [state, setState] = useState<RecordingState>(RecordingState.Idle);
  const [hasPermissions, setHasPermissions] = useState<boolean>(false);

  useEffect(() => {
    const pauseListener = RecordingNotificationManager.addEventListener(
      'recordingNotificationPause',
      () => {
        console.log('Notification pause action received');
        onPauseRecording();
      }
    );

    const resumeListener = RecordingNotificationManager.addEventListener(
      'recordingNotificationResume',
      () => {
        console.log('Notification resume action received');
        onResumeRecording();
      }
    );

    return () => {
      pauseListener.remove();
      resumeListener.remove();
      RecordingNotificationManager.hide();
    };
  }, []);

  const setNotification = (paused: boolean) => {
    RecordingNotificationManager.show({
      title: 'Recording Demo',
      contentText: paused ? 'Paused recording' : 'Recording...',
      paused,
      smallIconResourceName: 'logo',
      pauseIconResourceName: 'pause',
      resumeIconResourceName: 'resume',
      color: 0xff6200,
    });
  };

  const onStartRecording = useCallback(async () => {
    if (state !== RecordingState.Idle) {
      return;
    }

    setState(RecordingState.Loading);

    if (!hasPermissions) {
      const permissionStatus = await AudioManager.requestRecordingPermissions();

      if (permissionStatus !== 'Granted') {
        Alert.alert('Error', "Recording permissions are no't granted");
        return;
      }

      setHasPermissions(true);
    }

    const success = await AudioManager.setAudioSessionActivity(true);

    if (!success) {
      Alert.alert('Error', 'Failed to activate audio session for recording.');
      return;
    }

    const result = Recorder.start();
    setNotification(false);

    if (result.status === 'success') {
      console.log('Recording started, file path:', result.path);
      setState(RecordingState.Recording);
      return;
    }

    console.log('Recording start error:', result);
    Alert.alert('Error', `Failed to start recording: ${result.message}`);
    setState(RecordingState.Idle);
  }, [state, hasPermissions]);

  const onPauseRecording = useCallback(() => {
    Recorder.pause();
    setNotification(true);
    setState(RecordingState.Paused);
  }, []);

  const onResumeRecording = useCallback(() => {
    Recorder.resume();
    setNotification(false);
    setState(RecordingState.Recording);
  }, []);

  const onStopRecording = useCallback(() => {
    Recorder.stop();
    RecordingNotificationManager.hide();
    setState(RecordingState.ReadyToPlay);
  }, []);

  const onPlayRecording = useCallback(() => {
    if (state !== RecordingState.ReadyToPlay) {
      return;
    }

    setState(RecordingState.Playing);
  }, [state]);

  const onToggleState = useCallback(
    (action: RecordingState) => {
      if (state === RecordingState.Paused) {
        if (action === RecordingState.Recording) {
          onResumeRecording();
          return;
        }
      }

      if (action === RecordingState.Recording) {
        onStartRecording();
        return;
      }

      if (action === RecordingState.Paused) {
        onPauseRecording();
        return;
      }

      if (action === RecordingState.Idle) {
        if (state === RecordingState.Recording) {
          onStopRecording();
        } else if (state === RecordingState.Playing) {
          setState(RecordingState.Idle);
        }
        return;
      }

      if (action === RecordingState.ReadyToPlay) {
        onStopRecording();
        return;
      }

      if (action === RecordingState.Playing) {
        onPlayRecording();
      }
    },
    [
      state,
      onStartRecording,
      onPauseRecording,
      onStopRecording,
      onResumeRecording,
      onPlayRecording,
    ]
  );

  useEffect(() => {
    (async () => {
      const permissionStatus = await AudioManager.checkRecordingPermissions();

      if (permissionStatus === 'Granted') {
        setHasPermissions(true);
      }
    })();
  }, []);

  useEffect(() => {
    Recorder.enableFileOutput();

    return () => {
      Recorder.disableFileOutput();
    };
  }, []);

  return (
    <Container disablePadding>
      <Status state={state} />
      <View style={styles.spacerM} />
      <RecordingTime state={state} />
      <View style={styles.spacerS} />
      <RecordingVisualization state={state} />
      <View style={styles.spacerM} />
      <ControlPanel state={state} onToggleState={onToggleState} />
    </Container>
  );
};

export default Record;

const styles = StyleSheet.create({
  spacerM: { height: 24 },
  spacerS: { height: 12 },
});
