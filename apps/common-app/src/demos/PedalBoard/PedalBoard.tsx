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
  AudioBufferSourceNode,
  AudioBuffer,
  GainNode,
  AudioRecorder,
  AudioManager,
} from 'react-native-audio-api';

import { Container, RoundButton } from '../../components';
import { audioContext } from '../../singletons';
import { colors } from '../../styles';
import AutoWahPedal from './AutoWahPedal';
import OverdrivePedal from './OverdrivePedal';
import ReverbPedal from './ReverbPedal';

const URL = 'https://files.catbox.moe/xbj6gn.flac';
const PEDALS = [
  { id: 0, component: OverdrivePedal },
  { id: 1, component: AutoWahPedal },
  { id: 2, component: ReverbPedal },
];

let permissionsGranted = false;

export default function PedalBoard() {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isRecording, setIsRecording] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);

  const sourceNodeRef = useRef<AudioBufferSourceNode | AudioRecorder>(null);
  const recorderRef = useRef<AudioRecorder | null>(null);
  const pedalInputNodesRef = useRef<GainNode[]>([]);
  const pedalOutputNodesRef = useRef<GainNode[]>([]);

  useEffect(() => {
    const init = async () => {
      setIsLoading(true);
      AudioManager.setSystemOptions({
        ios: {
          category: 'playAndRecord',
          mode: 'default',
        },
      });
      AudioManager.setAudioSessionActivity(true);

      try {
        // Load audio file
        const audioBuffer = await fetch(URL, {
          headers: {
            'User-Agent':
              'Mozilla/5.0 (Android; Mobile; rv:122.0) Gecko/122.0 Firefox/122.0',
          },
        })
          .then((response) => response.arrayBuffer())
          .then((arrayBuffer) => audioContext.decodeAudioData(arrayBuffer));

        for (let i = 0; i < PEDALS.length; i++) {
          const input = audioContext.createGain();
          const output = audioContext.createGain();
          pedalInputNodesRef.current.push(input);
          pedalOutputNodesRef.current.push(output);
        }
        setAudioBuffer(audioBuffer);
        sourceNodeRef.current = audioContext.createBufferSource();
        sourceNodeRef.current.buffer = audioBuffer;
        handleConnections();
      } catch (error) {
        console.error('Error loading audio:', error);
      } finally {
        setIsLoading(false);
      }
    };

    init();
    return () => {
      sourceNodeRef.current?.disconnect();
    };
  }, []);

  const handleConnections = () => {
    if (!sourceNodeRef.current) {
      return;
    }
    const sourceNode = sourceNodeRef.current;
    if (sourceNode instanceof AudioRecorder) {
      sourceNode.connect(audioContext, pedalInputNodesRef.current[0]);
    } else {
      sourceNode.connect(pedalInputNodesRef.current[0]);
    }
    for (let i = 0; i < PEDALS.length; i++) {
      pedalInputNodesRef.current[i].connect(pedalOutputNodesRef.current[i]);
    }

    for (let i = 0; i < PEDALS.length - 1; i++) {
      pedalOutputNodesRef.current[i].connect(pedalInputNodesRef.current[i + 1]);
    }
    pedalOutputNodesRef.current[PEDALS.length - 1].connect(
      audioContext.destination
    );
  };

  const togglePlayback = () => {
    if (isPlaying) {
      sourceNodeRef.current?.disconnect();
      setIsPlaying(false);
    } else {
      if (!audioBuffer) {
        return;
      }
      sourceNodeRef.current = audioContext.createBufferSource();
      sourceNodeRef.current.buffer = audioBuffer;
      sourceNodeRef.current.connect(pedalInputNodesRef.current[0]);
      sourceNodeRef.current.start();
      setIsPlaying(true);
    }
  };

  const toggleRecording = async () => {
    if (isRecording) {
      await recorderRef.current?.stop();
      sourceNodeRef.current?.disconnect();
      setIsRecording(false);
    } else {
      if (!permissionsGranted) {
        const recPerm = await AudioManager.requestRecordingPermissions();
        permissionsGranted = recPerm === 'Granted';
      }
      if (permissionsGranted) {
        const recorder = new AudioRecorder();
        sourceNodeRef.current = recorder;
        recorderRef.current = recorder;
        recorder.connect(audioContext, pedalInputNodesRef.current[0]);
        if (audioContext.state === 'suspended') {
          audioContext.resume();
        }
        await recorder.start();
        setIsRecording(true);
      } else {
        Alert.alert(
          'Insufficient permissions!',
          'You need to grant audio recording permissions to use this feature.'
        );
      }
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
            {PEDALS.map((pedal, index) => {
              const PedalComponent = pedal.component;
              return (
                <PedalComponent
                  key={pedal.id}
                  context={audioContext}
                  inputNode={pedalInputNodesRef.current[index]}
                  outputNode={pedalOutputNodesRef.current[index]}
                />
              );
            })}
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
              color={colors.main}
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
    borderTopColor: colors.listSeparator,
  },
});
