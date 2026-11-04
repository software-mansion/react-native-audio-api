import { useNavigation } from '@react-navigation/native';
import React, {
  FC,
  useCallback,
  useEffect,
  useLayoutEffect,
  useRef,
  useState,
} from 'react';
import {
  AudioManager,
  AudioRecorder,
  concatAudioFiles,
  FileFormat,
  RecordingNotificationManager,
  useAudioInput,
} from 'react-native-audio-api';

import { Alert, ScrollView, StyleSheet } from 'react-native';

import { Container } from '../../components';
import { useControlsSheet } from '../../navigation/useControlsSheet';
import { audioRecorder as Recorder, audioContext } from '../../singletons';
import { layout } from '../../styles';
import RecordingsList from './RecordingsList';
import { addRecording } from './recordingsStore';
import {
  recorderSheetStore,
  useRecorderSheetState,
} from './recorderSheetStore';
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

const Record: FC = () => {
  const navigation = useNavigation('RecordDemo');

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
  const stateRef = useRef(state);
  const { availableInputs, currentInput, selectInput } = useAudioInput();
  const { sheetHeight } = useRecorderSheetState();

  useControlsSheet('RecordDemoControls');

  useLayoutEffect(() => {
    const onSelectInput = async (input: (typeof availableInputs)[number]) => {
      try {
        await selectInput(input);
      } catch (error) {
        Alert.alert('Input Device Error', `${error}`);
      }
    };

    navigation.setOptions({
      unstable_headerRightItems: () => [
        {
          type: 'menu',
          label: 'Input',
          icon: { type: 'sfSymbol', name: 'mic' },
          accessibilityLabel: `Input: ${currentInput?.name ?? 'System default'}`,
          menu: {
            title: 'Input',
            items:
              availableInputs.length > 0
                ? availableInputs.map((device) => ({
                    type: 'action' as const,
                    label: device.name,
                    description: device.category,
                    state:
                      device.id === currentInput?.id
                        ? ('on' as const)
                        : ('off' as const),
                    onPress: () => onSelectInput(device),
                  }))
                : [
                    {
                      type: 'action' as const,
                      label: 'System default',
                      state: 'on' as const,
                      disabled: true,
                      onPress: () => {},
                    },
                  ],
          },
        },
      ],
    });
  }, [navigation, availableInputs, currentInput, selectInput]);

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

    setState(RecordingState.Starting);

    if (!hasPermissions) {
      const permissionStatus = await AudioManager.requestRecordingPermissions();

      if (permissionStatus !== 'Granted') {
        Alert.alert('Error', "Recording permissions aren't granted");
        setState(RecordingState.Idle);
        return;
      }

      setHasPermissions(true);
    }

    AudioManager.setSystemOptions({
      ios: {
        category: 'playAndRecord',
        mode: 'default',
        options: [
          'defaultToSpeaker',
          'allowBluetoothA2DP',
          'allowBluetoothHFP',
          'mixWithOthers',
        ],
      },
    });

    try {
      await AudioManager.setAudioSessionActivity(true);
    } catch (error) {
      console.error(error);
      Alert.alert('Error', 'Failed to activate audio session for recording.');
      setState(RecordingState.Idle);
      return;
    }

    const result = await Recorder.start();

    if (result.status === 'success') {
      setupNotification(false);
      setIsInterrupted(false);
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
    setIsInterrupted(false);
    setState(RecordingState.Paused);
  }, []);

  const onResumeRecording = useCallback(() => {
    Recorder.resume();
    updateNotification(false);
    setState(RecordingState.Recording);
  }, []);

  const loadRecordedAudio = useCallback(async (paths: string[]) => {
    setState(RecordingState.Saving);

    const extension = RECORDING_EXTENSION_NAME_MAP[RECORDING_EXTENSION];
    const finalPath =
      paths.length > 1
        ? await concatAudioFiles(
            paths,
            paths[0].replace(/[^/]+$/, `recording.${extension}`)
          )
        : paths[0];

    const audioBuffer = await audioContext.decodeAudioData(finalPath);
    addRecording(audioBuffer);
    setState(RecordingState.Idle);
  }, []);

  const onStopRecording = useCallback(async () => {
    const info = await Recorder.stop();
    AudioRecorder.consumeLastRecordingResult();
    RecordingNotificationManager.hide();
    setIsInterrupted(false);
    setState(RecordingState.Saving);

    if (info.status !== 'success') {
      Alert.alert('Error', `Failed to stop recording: ${info.message}`);
      setState(RecordingState.Idle);
      return;
    }

    await loadRecordedAudio(info.paths);
  }, [loadRecordedAudio]);

  const onStopRecordingFromNotification = useCallback(async () => {
    const info = AudioRecorder.consumeLastRecordingResult();

    if (!info || info.paths.length === 0) {
      setState(RecordingState.Idle);
      return;
    }

    await loadRecordedAudio(info.paths);
  }, [loadRecordedAudio]);

  const onRecordPress = useCallback(() => {
    if (state === RecordingState.Idle) {
      onStartRecording();
    } else if (
      state === RecordingState.Recording ||
      state === RecordingState.Paused
    ) {
      onStopRecording();
    }
  }, [state, onStartRecording, onStopRecording]);

  const onPausePress = useCallback(() => {
    if (state === RecordingState.Recording) {
      onPauseRecording();
    } else if (state === RecordingState.Paused) {
      onResumeRecording();
    }
  }, [state, onPauseRecording, onResumeRecording]);

  useEffect(() => {
    stateRef.current = state;
  }, [state]);

  useEffect(() => {
    recorderSheetStore.publish({
      state,
      isInterrupted,
      onRecordPress,
      onPausePress,
    });
  }, [state, isInterrupted, onRecordPress, onPausePress]);

  useEffect(() => {
    (async () => {
      const recordingPermissionStatus =
        await AudioManager.checkRecordingPermissions();

      if (recordingPermissionStatus === 'Granted') {
        setHasPermissions(true);
      }

      const notificationPermissionStatus =
        await AudioManager.checkNotificationPermissions();
      if (notificationPermissionStatus !== 'Granted') {
        const result = await AudioManager.requestNotificationPermissions();
        if (result !== 'Granted') {
          console.warn('Notification permissions are not granted');
        }
      }
    })();
  }, []);

  useEffect(() => {
    AudioManager.observeAudioInterruptions(true);

    const interruptionSubscription = AudioManager.addSystemEventListener(
      'interruption',
      (event) => {
        if (event.type === 'began') {
          if (stateRef.current === RecordingState.Recording) {
            setIsInterrupted(true);
          }
          return;
        }

        if (event.type === 'ended') {
          setIsInterrupted(false);
        }
      }
    );

    return () => {
      interruptionSubscription.remove();
      AudioManager.observeAudioInterruptions(false);
    };
  }, []);

  useEffect(() => {
    Recorder.onError((error) => {
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

  // Collect the files of a recording that was stopped natively while this screen was unmounted.
  useEffect(() => {
    if (AudioRecorder.isRecordingOngoing()) {
      return;
    }

    const info = AudioRecorder.consumeLastRecordingResult();
    if (info && info.paths.length > 0) {
      loadRecordedAudio(info.paths);
    }
  }, [loadRecordedAudio]);

  useEffect(() => {
    if (!AudioRecorder.isRecordingOngoing()) {
      Recorder.enableFileOutput({
        rotateIntervalBytes: ROTATING_SIZE,
        format: RECORDING_EXTENSION,
        fileName: 'my_recording',
      });
    }

    return () => {
      if (!AudioRecorder.isRecordingOngoing()) {
        AudioManager.setAudioSessionActivity(false);
      }
    };
  }, []);

  const isTakeRunning =
    state === RecordingState.Recording || state === RecordingState.Paused;

  return (
    <Container disablePadding>
      <ScrollView
        contentContainerStyle={[
          styles.content,
          { paddingBottom: sheetHeight + layout.screenPadding },
        ]}
      >
        <RecordingsList disabled={isTakeRunning} />
      </ScrollView>
    </Container>
  );
};

export default Record;

const styles = StyleSheet.create({
  content: {
    flexGrow: 1,
    paddingHorizontal: layout.screenPadding,
  },
});
