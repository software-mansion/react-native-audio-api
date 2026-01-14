import React, { FC, useCallback, useEffect, useState } from 'react';
import { AudioBuffer, AudioManager } from 'react-native-audio-api';

import { Alert, StyleSheet, View } from 'react-native';
import { Container } from '../../components';

import { Easing, useSharedValue, withTiming } from 'react-native-reanimated';
import { useSafeAreaInsets } from 'react-native-safe-area-context';
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
  const playbackPosition = useSharedValue<number>(0);
  const [durationSeconds, setDurationSeconds] = useState<number>(0);
  const [playbackBuffer, setPlaybackBuffer] = useState<AudioBuffer | null>(
    null
  );
  const insets = useSafeAreaInsets();

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
    setState(RecordingState.Paused);
  }, []);

  const onResumeRecording = useCallback(() => {
    Recorder.resume();
    setState(RecordingState.Recording);
  }, []);

  const onStopRecording = useCallback(async () => {
    const info = Recorder.stop();

    if (info.status !== 'success') {
      Alert.alert('Error', `Failed to stop recording: ${info.message}`);
      setState(RecordingState.Idle);
      return;
    }

    const buffer = await audioContext.decodeAudioData(info.path);

    setDurationSeconds(buffer.duration);
    setPlaybackBuffer(buffer);
    setState(RecordingState.ReadyToPlay);
  }, []);

  const onPlayRecording = useCallback(() => {
    if (state !== RecordingState.ReadyToPlay) {
      return;
    }

    const sourceNode = audioContext.createBufferSource();
    sourceNode.buffer = playbackBuffer!;
    sourceNode.connect(audioContext.destination);
    sourceNode.start(audioContext.currentTime + 0.1);

    sourceNode.onEnded = () => {
      setState(RecordingState.Idle);
      setDurationSeconds(0);
      setPlaybackBuffer(null);
      playbackPosition.value = 0;
    };

    const duration = playbackBuffer!.duration;

    setTimeout(() => {
      playbackPosition.value = 0;
      playbackPosition.value = withTiming(duration, {
        duration: duration * 1000,
        easing: Easing.linear,
      });
    }, 100);

    setState(RecordingState.Playing);
  }, [state, playbackBuffer, playbackPosition]);

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
          setDurationSeconds(0);
          setPlaybackBuffer(null);
          playbackPosition.value = 0;
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
      playbackPosition,
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

  useEffect(() => {
    return () => {
      if (Recorder.isRecording()) {
        Recorder.stop();
      }
    };
  }, []);

  return (
    <Container disablePadding>
      <Status state={state} />
      <View style={styles.spacerM} />

      {[RecordingState.ReadyToPlay, RecordingState.Playing].includes(state) ? (
        <>
          <PlaybackVisualization
            buffer={playbackBuffer}
            currentPositionSeconds={playbackPosition}
            durationSeconds={durationSeconds}
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
      <View style={{ height: insets.bottom + insets.top }} />
    </Container>
  );
};

export default Record;

const styles = StyleSheet.create({
  spacerM: { height: 24 },
  spacerS: { height: 12 },
});
