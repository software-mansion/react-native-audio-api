import React, { FC, useCallback, useEffect, useRef, useState } from 'react';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioManager,
  AudioRecorder,
  concatAudioFiles,
  FileFormat,
  RecordingNotificationManager,
  useAudioInput,
} from 'react-native-audio-api';

import { Alert, AppState, StyleSheet, View } from 'react-native';
import { Container, Select } from '../../components';

import { Easing, useSharedValue, withTiming } from 'react-native-reanimated';
import { audioRecorder as Recorder, audioContext } from '../../singletons';
import ControlPanel from './ControlPanel';
import PlaybackVisualization from './PlaybackVisualization';
import RecordingTime from './RecordingTime';
import RecordingVisualization from './RecordingVisualization';
import Status from './Status';
import { RecordingState } from './types';

// concatAudioFiles supports WAV, M4A, FLAC, ADTS, and OGG (Opus) — the formats
// recordable on both iOS and Android.
const RECORDING_EXTENSION = FileFormat.M4A;
const ROTATING_SIZE = 250_000;

const RECORDING_EXTENSION_NAME_MAP = {
  [FileFormat.Wav]: 'wav',
  [FileFormat.M4A]: 'm4a',
  [FileFormat.Flac]: 'flac',
  [FileFormat.OpusOgg]: 'ogg',
  [FileFormat.Adts]: 'aac',
};

function log(message: string): void {
  console.log(`[record] ${message}`);
}

const Record: FC = () => {
  // Recover from "app disabled" state - recording can survive the app kill (android)
  const [state, setState] = useState<RecordingState>(() => {
    if (!AudioRecorder.isRecordingOngoing()) {
      return RecordingState.Idle;
    }
    return Recorder.isPaused()
      ? RecordingState.Paused
      : RecordingState.Recording;
  });
  const [hasPermissions, setHasPermissions] = useState<boolean>(false);
  const [isInterrupted, setIsInterrupted] = useState(false);
  const [recordedBuffer, setRecordedBuffer] = useState<AudioBuffer | null>(
    null
  );
  const currentPositionSV = useSharedValue(0);
  const playbackSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const stateRef = useRef(state);
  const { availableInputs, currentInput, selectInput } = useAudioInput();

  const inputLabels = availableInputs.map(
    (device) => `${device.name} · ${device.category} #${device.id}`
  );
  const currentInputLabel = currentInput
    ? inputLabels[availableInputs.indexOf(currentInput)]
    : 'System default';

  const onSelectInputLabel = async (label: string) => {
    const input = availableInputs[inputLabels.indexOf(label)];

    if (!input) {
      return;
    }

    log(`selectInput ${label}`);
    try {
      await selectInput(input);
      log(`selectInput ${label}: ok`);
    } catch (error) {
      log(`selectInput ${label}: failed: ${error}`);
      Alert.alert('Input Device Error', `${error}`);
    }
  };

  const stopPlayback = useCallback(() => {
    const source = playbackSourceRef.current;
    if (source) {
      log('stop playback');
      source.onended = null;
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
      color: 0xff6200,
      showStopAction: true,
      deepLinkUri: 'audioapi-example://record',
      usesChronometer: true,
    });
  };

  const onStartRecording = useCallback(async () => {
    if (state !== RecordingState.Idle) {
      return;
    }

    log('start recording');
    setState(RecordingState.Loading);

    if (!hasPermissions) {
      const permissionStatus = await AudioManager.requestRecordingPermissions();
      log(`AudioManager.requestRecordingPermissions() → ${permissionStatus}`);

      if (permissionStatus !== 'Granted') {
        Alert.alert('Error', "Recording permissions are no't granted");
        setState(RecordingState.Idle);
        return;
      }

      setHasPermissions(true);
    }

    const systemOptions = {
      ios: {
        category: 'playAndRecord' as const,
        mode: 'default' as const,
        options: [
          'defaultToSpeaker' as const,
          'allowBluetoothA2DP' as const,
          'mixWithOthers' as const,
        ],
      },
    };
    log(`AudioManager.setSystemOptions(${JSON.stringify(systemOptions)})`);
    AudioManager.setSystemOptions(systemOptions);

    try {
      await AudioManager.setAudioSessionActivity(true);
      log('AudioManager.setAudioSessionActivity(true): ok');
    } catch (error) {
      log(`AudioManager.setAudioSessionActivity(true): failed: ${error}`);
      console.error(error);
      Alert.alert('Error', 'Failed to activate audio session for recording.');
      setState(RecordingState.Idle);
      return;
    }

    const result = await Recorder.start();
    log(`recorder.start() → ${JSON.stringify(result)}`);

    setupNotification(false);

    if (result.status === 'success') {
      setIsInterrupted(false);
      setState(RecordingState.Recording);
      return;
    }

    Alert.alert('Error', `Failed to start recording: ${result.message}`);
    setState(RecordingState.Idle);
  }, [state, hasPermissions]);

  const onPauseRecording = useCallback(() => {
    log('recorder.pause()');
    Recorder.pause();
    updateNotification(true);
    setIsInterrupted(false);
    setState(RecordingState.Paused);
  }, []);

  const onResumeRecording = useCallback(() => {
    log('recorder.resume()');
    Recorder.resume();
    updateNotification(false);
    setState(RecordingState.Recording);
  }, []);

  const loadRecordedAudio = useCallback(
    async (paths: string[]) => {
      setState(RecordingState.Loading);

      const extension = RECORDING_EXTENSION_NAME_MAP[RECORDING_EXTENSION];
      const finalPath =
        paths.length > 1
          ? await concatAudioFiles(
              paths,
              paths[0].replace(/[^/]+$/, `recording.${extension}`)
            )
          : paths[0];
      log(
        paths.length > 1
          ? `concat ${paths.length} files → ${finalPath}`
          : `take ${finalPath}`
      );

      log(`decode ${finalPath}`);
      const audioBuffer = await audioContext.decodeAudioData(finalPath);
      log(
        `decoded ${audioBuffer.duration.toFixed(2)} s, ${audioBuffer.numberOfChannels} ch`
      );
      setRecordedBuffer(audioBuffer);

      setState(RecordingState.ReadyToPlay);
      currentPositionSV.value = 0;
    },
    [currentPositionSV]
  );

  const onStopRecording = useCallback(async () => {
    log('recorder.stop()');
    const info = await Recorder.stop();
    log(`recorder.stop() → ${JSON.stringify(info)}`);
    AudioRecorder.consumeLastRecordingResult();
    RecordingNotificationManager.hide();
    setIsInterrupted(false);
    setState(RecordingState.Loading);

    if (info.status !== 'success') {
      Alert.alert('Error', `Failed to stop recording: ${info.message}`);
      setRecordedBuffer(null);
      setState(RecordingState.Idle);
      return;
    }

    await loadRecordedAudio(info.paths);
  }, [loadRecordedAudio]);

  const onStopRecordingFromNotification = useCallback(async () => {
    const info = AudioRecorder.consumeLastRecordingResult();
    log(
      `notification stop, consumeLastRecordingResult() → ${JSON.stringify(info)}`
    );

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
    // GC the HostObject before onended fires; its destructor clears the native
    // callback id and the UI never leaves the Playing state.
    playbackSourceRef.current = source;

    source.onended = () => {
      log('playback ended');
      stopPlayback();
      setState(RecordingState.Idle);
    };

    log(`play ${recordedBuffer.duration.toFixed(2)} s`);
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
    stateRef.current = state;
    log(`state ${state}`);
  }, [state]);

  useEffect(() => {
    log(
      `mount, AudioRecorder.isRecordingOngoing() → ${AudioRecorder.isRecordingOngoing()}, state ${stateRef.current}`
    );

    const appStateSubscription = AppState.addEventListener('change', (next) =>
      log(`AppState '${next}'`)
    );

    (async () => {
      const recordingPermissionStatus =
        await AudioManager.checkRecordingPermissions();
      log(
        `AudioManager.checkRecordingPermissions() → ${recordingPermissionStatus}`
      );

      if (recordingPermissionStatus === 'Granted') {
        setHasPermissions(true);
      }

      const notificationPermissionStatus =
        await AudioManager.checkNotificationPermissions();
      log(
        `AudioManager.checkNotificationPermissions() → ${notificationPermissionStatus}`
      );
      if (notificationPermissionStatus !== 'Granted') {
        const result = await AudioManager.requestNotificationPermissions();
        log(`AudioManager.requestNotificationPermissions() → ${result}`);
        if (result !== 'Granted') {
          console.warn('Notification permissions are not granted');
        }
      }
    })();

    return () => {
      appStateSubscription.remove();
      log('unmount');
    };
  }, []);

  useEffect(() => {
    AudioManager.observeAudioInterruptions(true);
    log('AudioManager.observeAudioInterruptions(true)');

    const subscriptions = [
      AudioManager.addSystemEventListener('interruption', (event) => {
        log(`'interruption' ${JSON.stringify(event)}`);
        if (event.type === 'began') {
          if (stateRef.current === RecordingState.Recording) {
            setIsInterrupted(true);
          }
          return;
        }

        if (event.type === 'ended') {
          setIsInterrupted(false);
        }
      }),
      AudioManager.addSystemEventListener('routeChange', (event) =>
        log(`'routeChange' ${JSON.stringify(event)}`)
      ),
      AudioManager.addSystemEventListener('duck', () => log("'duck'")),
    ];

    return () => {
      subscriptions.forEach((subscription) => subscription.remove());
      AudioManager.observeAudioInterruptions(false);
    };
  }, []);

  useEffect(() => {
    Recorder.onError((error) => {
      log(`recorder.onError: ${error.message}`);
      Alert.alert('Recorder Error', error.message);
    });

    return () => {
      Recorder.clearOnError();
    };
  }, []);

  useEffect(() => {
    const pauseListener = RecordingNotificationManager.addEventListener(
      'recordingNotificationPause',
      () => {
        log('notification pause');
        onPauseRecording();
      }
    );

    const resumeListener = RecordingNotificationManager.addEventListener(
      'recordingNotificationResume',
      () => {
        log('notification resume');
        onResumeRecording();
      }
    );

    const stopListener = RecordingNotificationManager.addEventListener(
      'recordingNotificationStop',
      () => {
        log('notification stop');
        onStopRecordingFromNotification();
      }
    );

    return () => {
      pauseListener.remove();
      resumeListener.remove();
      stopListener.remove();
    };
  }, [onPauseRecording, onResumeRecording, onStopRecordingFromNotification]);

  // Collect the files of a recording that was stopped natively while this screen was unmounted.
  useEffect(() => {
    if (AudioRecorder.isRecordingOngoing()) {
      return;
    }

    const info = AudioRecorder.consumeLastRecordingResult();
    if (info && info.paths.length > 0) {
      log(`recovered stopped recording ${JSON.stringify(info.paths)}`);
      loadRecordedAudio(info.paths);
    }
  }, [loadRecordedAudio]);

  useEffect(() => {
    if (!AudioRecorder.isRecordingOngoing()) {
      const fileOutput = {
        rotateIntervalBytes: ROTATING_SIZE,
        format: RECORDING_EXTENSION,
        fileName: 'my_recording',
      };
      log(`recorder.enableFileOutput(${JSON.stringify(fileOutput)})`);
      Recorder.enableFileOutput(fileOutput);
    }

    return () => {
      stopPlayback();

      if (!AudioRecorder.isRecordingOngoing()) {
        log('AudioManager.setAudioSessionActivity(false)');
        AudioManager.setAudioSessionActivity(false);
      }
    };
  }, [stopPlayback]);

  return (
    <Container disablePadding>
      <View style={styles.inputPicker}>
        <Select
          value={currentInputLabel}
          options={inputLabels}
          onChange={onSelectInputLabel}
        />
      </View>
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
          <RecordingVisualization state={state} isInterrupted={isInterrupted} />
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
  inputPicker: { paddingHorizontal: 16, paddingTop: 12 },
});
