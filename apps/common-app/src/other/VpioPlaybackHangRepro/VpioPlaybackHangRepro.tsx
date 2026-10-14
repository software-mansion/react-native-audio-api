import React, { FC, useRef, useState } from 'react';
import { Alert, ScrollView, Text, View } from 'react-native';
import {
  AudioContext,
  AudioManager,
  AudioRecorder,
} from 'react-native-audio-api';

import { Button, Container } from '../../components';
import { colors } from '../../styles';

/**
 * Field report (nightly 1966b4a, 2026-09-18): a VPIO recorder is running, then a fresh
 * AudioContext({ sampleRate: 24000 }) is created and a buffer scheduled on it. Main hangs in
 * AURemoteIO::Initialize under applyVoiceProcessing → setVoiceProcessingEnabled: and the
 * watchdog kills the app. Each step is a separate button so the hanging one is visible.
 */

const PLAYBACK_SAMPLE_RATE = 24000;
const TONE_SECONDS = 0.5;
const TONE_HZ = 440;

type SessionOptions = Parameters<typeof AudioManager.setAudioSessionOptions>[0];

// V1–V3 request a session that voice processing does not run in (it rewrites the mode to
// voiceChat), which is what keeps the session-configuration fight alive. V4 already matches it
// and is the control.
const SESSION_PRESETS: { key: string; label: string; options: SessionOptions }[] = [
  {
    key: 'V1',
    label: 'V1: default, no options',
    options: { iosCategory: 'playAndRecord', iosMode: 'default', iosOptions: [] },
  },
  {
    key: 'V2',
    label: 'V2: default, defaultToSpeaker|allowBluetoothA2DP',
    options: {
      iosCategory: 'playAndRecord',
      iosMode: 'default',
      iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
    },
  },
  {
    key: 'V3',
    label: 'V3: iosMode omitted, defaultToSpeaker',
    options: { iosCategory: 'playAndRecord', iosOptions: ['defaultToSpeaker'] },
  },
  {
    key: 'V4',
    label: 'V4: voiceChat, defaultToSpeaker|allowBluetoothHFP (control)',
    options: {
      iosCategory: 'playAndRecord',
      iosMode: 'voiceChat',
      iosOptions: ['defaultToSpeaker', 'allowBluetoothHFP'],
    },
  },
];

const VpioPlaybackHangRepro: FC = () => {
  const recorderRef = useRef<AudioRecorder | null>(null);
  const contextRef = useRef<AudioContext | null>(null);
  const [log, setLog] = useState<string[]>([]);
  const [chunks, setChunks] = useState(0);
  const [presetKey, setPresetKey] = useState('V4');
  const preset = SESSION_PRESETS.find((candidate) => candidate.key === presetKey)!;

  const append = (line: string) =>
    setLog((prev) => [...prev.slice(-30), `${new Date().toISOString().slice(11, 23)} ${line}`]);

  const configureSession = async () => {
    AudioManager.setAudioSessionOptions(preset.options);
    await AudioManager.setAudioSessionActivity(true);
    append(`session ${preset.label}, active`);
  };

  const startRecorder = async () => {
    const permission = await AudioManager.requestRecordingPermissions();
    if (permission !== 'Granted') {
      Alert.alert('Recording permission denied');
      return;
    }

    const recorder = new AudioRecorder({
      iosVoiceProcessing: true,
      androidInputPreset: 'voiceCommunication',
    });
    recorder.onAudioReady(
      { sampleRate: 16000, bufferLength: 640, channelCount: 1 },
      () => setChunks((n) => n + 1)
    );

    const result = await recorder.start();
    recorderRef.current = recorder;
    append(`recorder.start → ${result.status}${result.status === 'error' ? `: ${result.message}` : ''}`);
  };

  const createContext = () => {
    if (contextRef.current) {
      append('context already exists');
      return;
    }
    append(`new AudioContext({ sampleRate: ${PLAYBACK_SAMPLE_RATE} }) …`);
    const context = new AudioContext({ sampleRate: PLAYBACK_SAMPLE_RATE });
    contextRef.current = context;
    append(`context created, state=${context.state}, sampleRate=${context.sampleRate}`);
  };

  const scheduleBuffer = () => {
    const context = contextRef.current;
    if (!context) {
      append('create the context first');
      return;
    }

    const frames = Math.floor(PLAYBACK_SAMPLE_RATE * TONE_SECONDS);
    const buffer = context.createBuffer(1, frames, PLAYBACK_SAMPLE_RATE);
    const samples = new Float32Array(frames);
    for (let i = 0; i < frames; i += 1) {
      samples[i] = 0.2 * Math.sin((2 * Math.PI * TONE_HZ * i) / PLAYBACK_SAMPLE_RATE);
    }
    buffer.copyToChannel(samples, 0);

    const source = context.createBufferSource();
    source.buffer = buffer;
    source.connect(context.destination);
    // If the context renders, currentTime advances by TONE_SECONDS between start and ended.
    // A tone that never ends, or ends with currentTime frozen, was never pulled by the engine.
    const startedAt = context.currentTime;
    source.onended = () =>
      append(
        `tone ended: currentTime ${startedAt.toFixed(3)} → ${context.currentTime.toFixed(3)}, state=${context.state}`
      );
    append(`source.start() at currentTime=${startedAt.toFixed(3)}, state=${context.state}`);
    source.start();
  };

  const runAll = async () => {
    await configureSession();
    await startRecorder();
    await new Promise((resolve) => setTimeout(resolve, 1500));
    createContext();
    scheduleBuffer();
  };

  // Ordering test for Bug 17: with the playback graph in place first, the VPIO output bus
  // has a mixer behind it from the first engine build. If the render errors disappear here
  // but not in runAll, the unfed output bus is what stops the engine.
  const runAllContextFirst = async () => {
    await configureSession();
    createContext();
    scheduleBuffer();
    await new Promise((resolve) => setTimeout(resolve, 1500));
    await startRecorder();
  };

  const teardown = async () => {
    await recorderRef.current?.stop();
    recorderRef.current = null;
    await contextRef.current?.close();
    contextRef.current = null;
    await AudioManager.setAudioSessionActivity(false);
    setChunks(0);
    append('torn down');
  };

  return (
    <Container centered={false}>
      <ScrollView contentContainerStyle={{ padding: 16, gap: 12 }}>
        <Text style={{ color: colors.white, fontSize: 13 }}>
          Field sequence: session → VPIO recorder → 24 kHz AudioContext → schedule buffer. Run
          the steps one at a time; the one that never returns is the hang.
        </Text>
        <Text style={{ color: colors.white }}>Session preset: {preset.label}</Text>
        <View style={{ flexDirection: 'row', flexWrap: 'wrap', gap: 8 }}>
          {SESSION_PRESETS.map((candidate) => (
            <Button
              key={candidate.key}
              title={candidate.key === presetKey ? `[${candidate.key}]` : candidate.key}
              onPress={() => setPresetKey(candidate.key)}
            />
          ))}
        </View>
        <Button title="1. Configure + activate session" onPress={configureSession} />
        <Button title="2. Start VPIO recorder" onPress={startRecorder} />
        <Button title="3. Create AudioContext (24 kHz)" onPress={createContext} />
        <Button title="4. Schedule buffer" onPress={scheduleBuffer} />
        <Button title="Run all (1.5 s between 2 and 3)" onPress={runAll} />
        <Button title="Run all, context first (1 → 3 → 4, 1.5 s, 2)" onPress={runAllContextFirst} />
        <Button title="Teardown" onPress={teardown} />
        <Text style={{ color: colors.white }}>recorder chunks: {chunks}</Text>
        <View style={{ backgroundColor: '#111', padding: 8, borderRadius: 6 }}>
          {log.map((line, index) => (
            <Text key={index} style={{ color: colors.white, fontSize: 11, fontFamily: 'Menlo' }}>
              {line}
            </Text>
          ))}
        </View>
      </ScrollView>
    </Container>
  );
};

export default VpioPlaybackHangRepro;
