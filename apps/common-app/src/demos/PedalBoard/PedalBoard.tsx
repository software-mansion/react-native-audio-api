import React, { useEffect, useRef, useState } from 'react';
import {
  ActivityIndicator,
  Alert,
  ScrollView,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioManager,
  AudioRecorder,
} from 'react-native-audio-api';

import { Container, RoundButton } from '../../components';
import { audioContext } from '../../singletons';
import { colors } from '../../styles';
import AutoWahPedal from './AutoWahPedal';
import OverdrivePedal from './OverdrivePedal';
import ReverbPedal from './ReverbPedal';

const LOOP_URL = 'https://files.catbox.moe/xbj6gn.flac';
const LOOP_FETCH_OPTIONS = {
  headers: {
    'User-Agent':
      'Mozilla/5.0 (Android; Mobile; rv:122.0) Gecko/122.0 Firefox/122.0',
  },
};
const PEDALS = [OverdrivePedal, AutoWahPedal, ReverbPedal];

let hasRecordingPermission = false;

export default function PedalBoard() {
  const [isLoading, setIsLoading] = useState(true);
  const [loop, setLoop] = useState<AudioBuffer | null>(null);
  const [isPlaying, setIsPlaying] = useState(false);
  const [isRecording, setIsRecording] = useState(false);

  const [recorder] = useState(() => new AudioRecorder());
  const [slots] = useState(() =>
    PEDALS.map(() => ({
      input: audioContext.createGain(),
      output: audioContext.createGain(),
    }))
  );
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);

  // Each pedal routes its own slot's input to its output; the board chains
  // the slots and owns every edge out of their outputs.
  useEffect(() => {
    slots.forEach(({ output }, index) =>
      output.connect(slots[index + 1]?.input ?? audioContext.destination)
    );

    return () => {
      const source = sourceRef.current;
      if (source) {
        source.onended = null;
        source.stop();
        source.disconnect();
      }
      if (recorder.isRecording()) {
        recorder.stop().catch((error) => console.warn(error));
      }
      recorder.disconnect();
      slots.forEach(({ output }) => output.disconnect());
    };
  }, [slots, recorder]);

  useEffect(() => {
    let isCancelled = false;
    AudioManager.setSystemOptions({
      ios: { category: 'playAndRecord', mode: 'default' },
    });
    AudioManager.setAudioSessionActivity(true);

    audioContext
      .decodeAudioData(LOOP_URL, LOOP_FETCH_OPTIONS)
      .then((buffer) => !isCancelled && setLoop(buffer))
      .catch((error) => console.error('Error loading audio:', error))
      .finally(() => !isCancelled && setIsLoading(false));

    return () => {
      isCancelled = true;
    };
  }, []);

  const startLoop = () => {
    if (!loop) {
      return;
    }
    const source = audioContext.createBufferSource();
    source.buffer = loop;
    source.connect(slots[0].input);
    source.onended = () => {
      source.disconnect();
      sourceRef.current = null;
      setIsPlaying(false);
    };
    source.start();
    sourceRef.current = source;
    setIsPlaying(true);
  };

  const togglePlayback = () =>
    isPlaying ? sourceRef.current?.stop() : startLoop();

  const toggleRecording = async () => {
    if (isRecording) {
      await recorder.stop().catch((error) => console.warn(error));
      recorder.disconnect();
      setIsRecording(false);
      return;
    }

    if (!hasRecordingPermission) {
      hasRecordingPermission =
        (await AudioManager.requestRecordingPermissions()) === 'Granted';
    }
    if (!hasRecordingPermission) {
      Alert.alert(
        'Insufficient permissions!',
        'You need to grant audio recording permissions to use this feature.'
      );
      return;
    }

    recorder.connect(audioContext, slots[0].input);
    if (audioContext.state === 'suspended') {
      audioContext.resume();
    }
    try {
      await recorder.start();
      setIsRecording(true);
    } catch (error) {
      recorder.disconnect();
      Alert.alert('Could not start the microphone', String(error));
    }
  };

  return (
    <Container disablePadding>
      {isLoading ? (
        <View style={styles.loading}>
          <ActivityIndicator color={colors.accent} />
          <Text style={styles.loadingText}>Loading guitar loop…</Text>
        </View>
      ) : (
        <>
          <ScrollView
            contentContainerStyle={styles.board}
            showsVerticalScrollIndicator={false}
          >
            {PEDALS.map((Pedal, index) => (
              <Pedal
                key={index}
                context={audioContext}
                inputNode={slots[index].input}
                outputNode={slots[index].output}
              />
            ))}
          </ScrollView>
          {/* Both sources feed the first pedal, so only one runs at a time. */}
          <View style={styles.transport}>
            <RoundButton
              icon={isPlaying ? 'stop' : 'play'}
              label={isPlaying ? 'Stop' : 'Guitar loop'}
              onPress={togglePlayback}
              disabled={isRecording}
              color={colors.accent}
            />
            <RoundButton
              icon={isRecording ? 'mic-off' : 'mic'}
              label={isRecording ? 'Stop' : 'Live mic'}
              onPress={toggleRecording}
              disabled={isPlaying}
              color={colors.blue}
              variant={isRecording ? 'solid' : 'outline'}
            />
          </View>
        </>
      )}
    </Container>
  );
}

const styles = StyleSheet.create({
  loading: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    gap: 12,
  },
  loadingText: {
    color: colors.textMuted,
    fontSize: 14,
  },
  board: {
    paddingHorizontal: 18,
    paddingBottom: 16,
    gap: 14,
  },
  transport: {
    flexDirection: 'row',
    justifyContent: 'center',
    gap: 48,
    paddingTop: 16,
    paddingBottom: 8,
    borderTopWidth: 1,
    borderTopColor: colors.track,
  },
});
