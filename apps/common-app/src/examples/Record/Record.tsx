import React, { FC, useEffect, useRef, useState } from 'react';
import { Alert, StyleSheet, Text } from 'react-native';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioManager,
  IOSMode,
  IOSOption,
  useAudioInput,
} from 'react-native-audio-api';

import { Button, Card, Container, Select } from '../../components';
import { audioContext, audioRecorder } from '../../singletons';
import { typography } from '../../styles';
import { activatePlaybackSession } from '../../utils/audio';

enum Status {
  Idle = 'Idle',
  LiveEcho = 'LiveEcho',
  Recording = 'Recording',
  Playback = 'Playback',
}

const REPLAY_RECORDING_MS = 5000;

async function verifyPermissions() {
  const recordingPermission = await AudioManager.requestRecordingPermissions();
  const notificationPermission =
    await AudioManager.requestNotificationPermissions();

  if (
    recordingPermission === 'Granted' &&
    notificationPermission === 'Granted'
  ) {
    return true;
  }

  Alert.alert(
    'Insufficient permissions!',
    'You need to grant audio recording permissions to use this feature.'
  );
  return false;
}

async function activateRecordingSession(mode: IOSMode, options: IOSOption[]) {
  AudioManager.setSystemOptions({
    ios: { category: 'playAndRecord', mode, options },
  });

  try {
    await AudioManager.setAudioSessionActivity(true);
    return true;
  } catch (error) {
    console.error('Failed to activate audio session:', error);
    Alert.alert(
      'Audio Session Error',
      'Failed to activate audio session for recording.'
    );
    return false;
  }
}

async function startRecorder() {
  try {
    await audioRecorder.start();
    return true;
  } catch (error) {
    Alert.alert(
      'Recording Error',
      `Failed to start recording: ${(error as Error).message}`
    );
    return false;
  }
}

const Record: FC = () => {
  const { availableInputs, currentInput, selectInput } = useAudioInput();

  const [status, setStatus] = useState<Status>(Status.Idle);
  const [capturedBuffers, setCapturedBuffers] = useState<AudioBuffer[]>([]);
  const sourceNodesRef = useRef<AudioBufferSourceNode[]>([]);
  const timeoutRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  /**
   * The recorder and context are app-wide singletons the Recorder demo also
   * uses, so unmounting only stops what this screen started.
   */
  const ownsRecorderRef = useRef(false);

  const scheduleTimeout = (callback: () => void, delayMs: number) => {
    if (timeoutRef.current) {
      clearTimeout(timeoutRef.current);
    }
    timeoutRef.current = setTimeout(callback, delayMs);
  };

  const releaseReplaySources = () => {
    sourceNodesRef.current.forEach((source) => source.disconnect());
    sourceNodesRef.current = [];
  };

  const startEcho = async () => {
    if (
      !(await verifyPermissions()) ||
      !(await activateRecordingSession('voiceChat', ['allowBluetoothHFP']))
    ) {
      return;
    }

    if (audioContext.state === 'suspended') {
      await audioContext.resume();
    }

    if (audioContext.state !== 'running') {
      Alert.alert(
        'Audio Context Error',
        `Audio context is not running. Current state: ${audioContext.state}`
      );
      return;
    }

    audioRecorder.connect(audioContext, audioContext.destination);
    ownsRecorderRef.current = true;

    if (await startRecorder()) {
      setStatus(Status.LiveEcho);
    }
  };

  /** Stops the recording and suspends the shared context, without closing it. */
  const stopEcho = async () => {
    await audioRecorder.stop();
    audioContext.suspend();

    audioRecorder.disconnect();
    ownsRecorderRef.current = false;
    setStatus(Status.Idle);
    await AudioManager.setAudioSessionActivity(false);
  };

  const startRecordForReplay = async () => {
    if (
      !(await verifyPermissions()) ||
      !(await activateRecordingSession('default', [
        'allowBluetoothA2DP',
        'allowBluetoothHFP',
      ]))
    ) {
      return;
    }

    setCapturedBuffers([]);

    const callbackResult = audioRecorder.onAudioReady(
      {
        sampleRate: audioContext.sampleRate,
        channelCount: 1,
        bufferLength: 4096,
      },
      ({ buffer }) => {
        setCapturedBuffers((prevBuffers) => [...prevBuffers, buffer]);
      }
    );

    if (callbackResult.status === 'error') {
      Alert.alert(
        'Recorder Error',
        `Failed to set audio ready callback: ${callbackResult.message}`
      );
      return;
    }

    ownsRecorderRef.current = true;
    if (!(await startRecorder())) {
      return;
    }

    setStatus(Status.Recording);

    scheduleTimeout(async () => {
      audioRecorder.stop();
      audioRecorder.clearOnAudioReady();
      ownsRecorderRef.current = false;
      await AudioManager.setAudioSessionActivity(false);
      setStatus(Status.Idle);
    }, REPLAY_RECORDING_MS);
  };

  const onStartReplay = async () => {
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

    if (audioContext.state === 'suspended') {
      await audioContext.resume();
    }

    releaseReplaySources();
    const replayStart = audioContext.currentTime;
    let nextStartAt = replayStart + 1;

    capturedBuffers.forEach((buffer) => {
      const source = audioContext.createBufferSource();
      source.buffer = buffer;
      source.connect(audioContext.destination);
      source.start(nextStartAt);
      nextStartAt += buffer.duration;
      sourceNodesRef.current.push(source);
    });

    setStatus(Status.Playback);

    scheduleTimeout(
      async () => {
        releaseReplaySources();
        await audioContext.suspend();
        await AudioManager.setAudioSessionActivity(false);
        setStatus(Status.Idle);
      },
      (nextStartAt - replayStart) * 1000
    );
  };

  const onSelect = async (id: string) => {
    const input = availableInputs.find((d) => d.id === id);

    if (!input) {
      return;
    }

    try {
      await selectInput(input);
    } catch (error) {
      // Android refuses a switch while a recorder is running, and either
      // platform refuses a device that went away between listing and picking.
      Alert.alert('Input Device Error', String(error));
    }
  };

  useEffect(() => {
    return () => {
      if (timeoutRef.current) {
        clearTimeout(timeoutRef.current);
      }
      sourceNodesRef.current.forEach((source) => source.disconnect());

      if (ownsRecorderRef.current) {
        audioRecorder.stop();
        audioRecorder.clearOnAudioReady();
        audioRecorder.disconnect();
      }
    };
  }, []);

  return (
    <Container style={styles.content}>
      <Text style={styles.status}>Status: {status}</Text>
      <Select
        value={currentInput?.id ?? ''}
        onChange={onSelect}
        options={availableInputs.map((d) => d.id)}
      />
      <Card style={styles.section}>
        <Text style={typography.sectionTitle}>Echo</Text>
        <Button title="Start Recording" onPress={startEcho} />
        <Button title="Stop Recording" onPress={stopEcho} />
      </Card>
      <Card style={styles.section}>
        <Text style={typography.sectionTitle}>Record & replay</Text>
        <Button title="Record for Replay" onPress={startRecordForReplay} />
        <Button title="Replay" onPress={onStartReplay} />
      </Card>
    </Container>
  );
};

export default Record;

const styles = StyleSheet.create({
  content: {
    gap: 24,
  },
  status: {
    ...typography.sectionTitle,
    textAlign: 'center',
  },
  section: {
    alignItems: 'center',
    gap: 12,
  },
});
