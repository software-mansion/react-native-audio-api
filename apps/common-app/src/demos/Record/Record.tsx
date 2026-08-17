import React, { FC, useCallback, useEffect, useRef, useState } from 'react';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioManager,
  AudioRecorder,
  // eslint-disable-next-line @typescript-eslint/no-unused-vars -- used by the commented-out concat flow above
  concatAudioFiles,
  FileFormat,
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

const Record: FC = () => {
  // A recording can outlive this screen (and, with `stopWithTask: false`, the whole
  // app UI). Mounting directly in the right state lets every child initialize from
  // the live recorder instead of transitioning out of a transient Idle render.
  const [state, setState] = useState<RecordingState>(() => {
    if (!AudioRecorder.isRecordingOngoing()) {
      return RecordingState.Idle;
    }
    return Recorder.isPaused()
      ? RecordingState.Paused
      : RecordingState.Recording;
  });
  const [hasPermissions, setHasPermissions] = useState<boolean>(false);
  const [recordedBuffer, setRecordedBuffer] = useState<AudioBuffer | null>(
    null
  );
  const currentPositionSV = useSharedValue(0);
  const playbackSourceRef = useRef<AudioBufferSourceNode | null>(null);

  const stopPlayback = useCallback(() => {
    const source = playbackSourceRef.current;
    if (source) {
      source.onEnded = null;
      source.disconnect();
      playbackSourceRef.current = null;
    }
    currentPositionSV.value = 0;
  }, [currentPositionSV]);

  const updateNotification = (paused: boolean) => {
    RecordingNotificationManager.show({
      paused,
    });
  };

  const setupNotification = (paused: boolean) => {
    RecordingNotificationManager.show({
      title: 'Recording Demo',
      contentText: paused ? 'Paused recording' : 'Recording...',
      paused,
      smallIconResourceName: 'logo',
      pauseIconResourceName: 'pause',
      resumeIconResourceName: 'resume',
      color: 0xff6200,
      showStopAction: true,
      stopIconResourceName: 'stop',
      deepLinkUri: 'audioapi-example://record',
      usesChronometer: true,
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

    AudioManager.setAudioSessionOptions({
      iosCategory: 'playAndRecord',
      iosMode: 'default',
      iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
    });

    try {
      await AudioManager.setAudioSessionActivity(true);
    } catch (error) {
      console.error(error);
      Alert.alert('Error', 'Failed to activate audio session for recording.');
      setState(RecordingState.Idle);
      return;
    }

    const result = await Recorder.start({
      fileNameOverride: `overridden_name_${Date.now()}`,
    });

    setupNotification(false);

    if (result.status === 'success') {
      setState(RecordingState.Recording);
      return;
    }

    console.log('Recording start error:', result);
    Alert.alert('Error', `Failed to start recording: ${result.message}`);
    setState(RecordingState.Idle);
  }, [state, hasPermissions]);

  const onPauseRecording = useCallback(() => {
    Recorder.pause();
    updateNotification(true);
    setState(RecordingState.Paused);
  }, []);

  const onResumeRecording = useCallback(() => {
    Recorder.resume();
    updateNotification(false);
    setState(RecordingState.Recording);
  }, []);

  const loadRecordedAudio = useCallback(
    async (paths: string[]) => {
      setState(RecordingState.Loading);

      // const outputPath = paths[0].replace(/[^/]+$/, 'recording.wav');

      // const finalPath = await concatAudioFiles(paths, outputPath);
      const finalPath = paths[0];
      const audioBuffer = await audioContext.decodeAudioData(finalPath);
      setRecordedBuffer(audioBuffer);

      setState(RecordingState.ReadyToPlay);
      currentPositionSV.value = 0;
    },
    [currentPositionSV]
  );

  const onStopRecording = useCallback(async () => {
    const info = await Recorder.stop();
    RecordingNotificationManager.hide();
    setState(RecordingState.Loading);

    if (info.status !== 'success') {
      Alert.alert('Error', `Failed to stop recording: ${info.message}`);
      setRecordedBuffer(null);
      setState(RecordingState.Idle);
      return;
    }

    await loadRecordedAudio(info.paths);
  }, [loadRecordedAudio]);

  // The stop action already stopped the recorder natively and hid the notification;
  // here we only pick up the resulting files and sync the UI.
  const onStopRecordingFromNotification = useCallback(async () => {
    const info = AudioRecorder.takeLastRecordingResult();

    if (!info || info.paths.length === 0) {
      setRecordedBuffer(null);
      setState(RecordingState.Idle);
      return;
    }

    await loadRecordedAudio(info.paths);
  }, [loadRecordedAudio]);

  const onPlayRecording = useCallback(() => {
    if (state !== RecordingState.ReadyToPlay) {
      return;
    }

    if (!recordedBuffer) {
      Alert.alert('Error', 'No recorded audio to play.');
      return;
    }

    stopPlayback();

    const source = audioContext.createBufferSource();
    source.buffer = recordedBuffer;
    source.connect(audioContext.destination);

    // Keep the source alive until playback ends. Without a ref, release builds can
    // GC the HostObject before onEnded fires; its destructor clears the native
    // callback id and the UI never leaves the Playing state.
    playbackSourceRef.current = source;

    source.onEnded = () => {
      stopPlayback();
      setState(RecordingState.Idle);
    };

    source.start(audioContext.currentTime);

    setTimeout(() => {
      currentPositionSV.value = withTiming(recordedBuffer.duration, {
        duration: recordedBuffer.duration * 1000,
        easing: Easing.linear,
      });
    }, 100);

    setState(RecordingState.Playing);
  }, [state, recordedBuffer, currentPositionSV, stopPlayback]);

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
          stopPlayback();
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
      stopPlayback,
    ]
  );

  useEffect(() => {
    (async () => {
      const recordingPermissionStatus = await AudioManager.checkRecordingPermissions();

      if (recordingPermissionStatus === 'Granted') {
        setHasPermissions(true);
      }

      const notificationPermissionStatus = await AudioManager.checkNotificationPermissions();
      if (notificationPermissionStatus !== 'Granted') {
        const result = await AudioManager.requestNotificationPermissions();
        if (result !== 'Granted') {
          console.warn('Notification permissions are not granted');
        }
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

    const stopListener = RecordingNotificationManager.addEventListener(
      'recordingNotificationStop',
      () => {
        console.log('Notification stop action received');
        onStopRecordingFromNotification();
      }
    );

    return () => {
      pauseListener.remove();
      resumeListener.remove();
      stopListener.remove();
    };
  }, [onPauseRecording, onResumeRecording, onStopRecordingFromNotification]);

  // An ongoing recording is picked up by the state initializer above; here we only
  // collect the files of a recording that was stopped natively (notification stop
  // action) while this screen was unmounted.
  useEffect(() => {
    if (AudioRecorder.isRecordingOngoing()) {
      return;
    }

    const info = AudioRecorder.takeLastRecordingResult();
    if (info && info.paths.length > 0) {
      loadRecordedAudio(info.paths);
    }
  }, [loadRecordedAudio]);

  useEffect(() => {
    // Re-enabling file output during an ongoing recording replaces the file writer,
    // which starts a new file and resets the duration — skip it when resyncing.
    if (!AudioRecorder.isRecordingOngoing()) {
      Recorder.enableFileOutput({ format: FileFormat.Wav });
    }

    return () => {
      // The recording and its notification intentionally stay alive when leaving this
      // screen; they can be stopped from the notification or after coming back.
      stopPlayback();
    };
  }, [stopPlayback]);

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
