import React, { FC, useCallback, useEffect, useState } from 'react';
import {
  AudioBuffer,
  AudioManager,
  RecordingNotificationManager,
} from 'react-native-audio-api';

import { Alert, StyleSheet, View } from 'react-native';
import { Container } from '../../components';

import { Easing, useSharedValue, withTiming } from 'react-native-reanimated';
import { audioRecorder as Recorder, audioContext } from '../../singletons';
import ControlPanel from './ControlPanel';
import PlaybackVisualization from './PlaybackVisualization';
import RecordingTime from './RecordingTime';
import RecordingVisualization from './RecordingVisualization';
import Status from './Status';
import { RecordingState } from './types';

AudioManager.setAudioSessionOptions({
  iosCategory: 'playAndRecord',
  iosMode: 'default',
  iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
});

const Record: FC = () => {
  const [state, setState] = useState<RecordingState>(RecordingState.Idle);
  const [hasPermissions, setHasPermissions] = useState<boolean>(false);
  const [recordedBuffer, setRecordedBuffer] = useState<AudioBuffer | null>(
    null
  );
  const currentPositionSV = useSharedValue(0);

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

  const onStopRecording = useCallback(async () => {
    const info = Recorder.stop();
    RecordingNotificationManager.hide();
    setState(RecordingState.ReadyToPlay);

    if (info.status !== 'success') {
      Alert.alert('Error', `Failed to stop recording: ${info.message}`);
      setRecordedBuffer(null);
      return;
    }

    const audioBuffer = await audioContext.decodeAudioData(info.path);
    setRecordedBuffer(audioBuffer);
  }, []);

  const onPlayRecording = useCallback(() => {
    if (state !== RecordingState.ReadyToPlay) {
      return;
    }

    if (!recordedBuffer) {
      Alert.alert('Error', 'No recorded audio to play.');
      return;
    }

    const source = audioContext.createBufferSource();
    source.buffer = recordedBuffer;
    source.connect(audioContext.destination);
    source.start(audioContext.currentTime + 0.1);

    source.onEnded = () => {
      setState(RecordingState.Idle);
    };

    setTimeout(() => {
      currentPositionSV.value = 0;

      withTiming(recordedBuffer.duration, {
        duration: recordedBuffer.duration * 1000,
        easing: Easing.linear,
      });
    }, 100);

    setState(RecordingState.Playing);
  }, [state, recordedBuffer, currentPositionSV]);

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
  }, [onPauseRecording, onResumeRecording]);

  useEffect(() => {
    Recorder.enableFileOutput();

    return () => {
      Recorder.disableFileOutput();
      Recorder.stop();
      AudioManager.setAudioSessionActivity(false);
      RecordingNotificationManager.hide();
    };
  }, []);

  return (
    <Container disablePadding>
      <Status state={state} />
      <View style={styles.spacerM} />
      {[RecordingState.Playing, RecordingState.ReadyToPlay].includes(state) ? (
        <>
          <PlaybackVisualization
            buffer={recordedBuffer}
            currentPositionSeconds={currentPositionSV}
            durationSeconds={recordedBuffer?.duration || 0}
          />
        </>
      ) : (
        <>
          <RecordingTime state={state} />
          <View style={styles.spacerS} />
          <RecordingVisualization state={state} />
        </>
      )}
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
