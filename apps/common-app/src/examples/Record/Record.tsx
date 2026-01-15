import React, { FC, useEffect, useState } from 'react';
import { Alert, Text, View } from 'react-native';
import {
  AudioBuffer,
  AudioManager,
} from 'react-native-audio-api';

import { Button, Container } from '../../components';
import { colors } from '../../styles';

import { audioContext, audioRecorder } from '../../singletons';

enum Status {
  Idle = 'Idle',
  LiveEcho = 'LiveEcho',
  Recording = 'Recording',
  Playback = 'Playback',
}

const Record: FC = () => {
  const [status, setStatus] = useState<Status>(Status.Idle);
  const [capturedBuffers, setCapturedBuffers] = useState<AudioBuffer[]>([]);

  const verifyPermissions = async () => {
    const recPerm = await AudioManager.requestRecordingPermissions();
    const notPerm = await AudioManager.requestNotificationPermissions();

    return recPerm === 'Granted' && notPerm === 'Granted';
  };

  const startEcho = async () => {
    const hasPermission = await verifyPermissions();
    if (!hasPermission) {
      Alert.alert(
        'Insufficient permissions!',
        'You need to grant audio recording permissions to use this feature.'
      );
      return;
    }

    AudioManager.setAudioSessionOptions({
      iosCategory: 'playAndRecord',
      iosMode: 'default',
      iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
    });

    const success = await AudioManager.setAudioSessionActivity(true);

    if (!success) {
      Alert.alert(
        'Audio Session Error',
        'Failed to activate audio session for recording.'
      );
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

    const adapter = audioContext.createRecorderAdapter();
    adapter.connect(audioContext.destination);
    audioRecorder.connect(adapter);

    const result = audioRecorder.start();

    if (result.status === 'error') {
      Alert.alert(
        'Recording Error',
        `Failed to start recording: ${result.message}`
      );
      return;
    }

    setStatus(Status.LiveEcho);
  };

  /// This stops only the recording, not the audio context
  const stopEcho = async () => {
    audioRecorder.stop();

    audioRecorder.disconnect();
    setStatus(Status.Idle);
    await AudioManager.setAudioSessionActivity(false);
  };

  const startRecordForReplay = async () => {
    const hasPermission = await verifyPermissions();

    if (!hasPermission) {
      Alert.alert(
        'Insufficient permissions!',
        'You need to grant audio recording permissions to use this feature.'
      );
      return;
    }

    AudioManager.setAudioSessionOptions({
      iosCategory: 'playAndRecord',
      iosMode: 'default',
      iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
    });

    const success = await AudioManager.setAudioSessionActivity(true);

    if (!success) {
      Alert.alert(
        'Audio Session Error',
        'Failed to activate audio session for recording.'
      );
      return;
    }

    setCapturedBuffers([]);

    const callbackResult = audioRecorder.onAudioReady(
      {
        sampleRate: audioContext.sampleRate,
        channelCount: 1,
        bufferLength: 4096,
      },
      (event) => {
        const { buffer, numFrames } = event;

        console.log('Audio recorder buffer ready:', buffer.duration, numFrames);
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

    setStatus(Status.Recording);

    setTimeout(async () => {
      audioRecorder.stop();
      audioRecorder.clearOnAudioReady();
      await AudioManager.setAudioSessionActivity(false);
      setStatus(Status.Idle);
    }, 5000);
  };

  const onStartReplay = async () => {
    AudioManager.setAudioSessionOptions({
      iosCategory: 'playback',
      iosMode: 'default',
      iosOptions: [],
    });

    const success = await AudioManager.setAudioSessionActivity(true);

    if (!success) {
      Alert.alert(
        'Audio Session Error',
        'Failed to activate audio session for playback.'
      );
      return;
    }

    if (audioContext.state === 'suspended') {
      audioContext.resume();
    }

    const tNow = audioContext.currentTime;
    let nextStartAt = tNow + 1;

    capturedBuffers.forEach((buffer) => {
      const source = audioContext.createBufferSource();
      source.buffer = buffer;
      source.connect(audioContext.destination);
      source.start(nextStartAt);
      nextStartAt += buffer.duration;
    });

    setStatus(Status.Playback);

    setTimeout(
      () => {
        setStatus(Status.Idle);
      },
      (nextStartAt - tNow) * 1000
    );
  };

  useEffect(() => {
    return () => {
      audioRecorder.stop();
    };
  }, []);

  return (
    <Container style={{ gap: 40 }}>
      <View style={{ alignItems: 'center' }}>
        <Text style={{ color: colors.white, fontSize: 20 }}>
          Status: {status}
        </Text>
      </View>
      <View style={{ alignItems: 'center', gap: 10 }}>
        <Text style={{ color: colors.white, fontSize: 16 }}>Echo</Text>
        <Button title="Start Recording" onPress={startEcho} />
        <Button title="Stop Recording" onPress={stopEcho} />
      </View>
      <View style={{ alignItems: 'center', gap: 10, paddingTop: 40 }}>
        <Text style={{ color: colors.white, fontSize: 16 }}>
          Record & replay
        </Text>
        <Button title="Record for Replay" onPress={startRecordForReplay} />
        <Button title="Replay" onPress={onStartReplay} />
      </View>
    </Container>
  );
};

export default Record;
