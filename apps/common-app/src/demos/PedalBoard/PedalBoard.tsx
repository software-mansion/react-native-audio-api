import React, { useEffect, useRef, useState } from 'react';
import {
  AudioBufferSourceNode,
  AudioBuffer,
  GainNode,
  RecorderAdapterNode,
  AudioRecorder,
  AudioManager
} from 'react-native-audio-api';
import { Container } from '../../components';
import { audioContext } from '../../singletons';
import { ActivityIndicator, View, Button, StyleSheet, ScrollView, Dimensions, Alert } from 'react-native';
import OverdrivePedal from './OverdrivePedal';
import ReverbPedal from './ReverbPedal';
import AutoWahPedal from './AutoWahPedal';

const screenWdith = Dimensions.get('window').width;

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

  const sourceNodeRef = useRef<AudioBufferSourceNode | RecorderAdapterNode>(null);
  const recorderRef = useRef<AudioRecorder | null>(null);
  const pedalInputNodesRef = useRef<GainNode[]>([]);
  const pedalOutputNodesRef = useRef<GainNode[]>([]);

  useEffect(() => {
    const init = async () => {
      setIsLoading(true);
      AudioManager.setAudioSessionOptions({
        iosCategory: 'playAndRecord',
        iosMode: 'default',
      })
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
    sourceNode.connect(pedalInputNodesRef.current[0]);
    for (let i = 0; i < PEDALS.length; i++) {
      pedalInputNodesRef.current[i].connect(pedalOutputNodesRef.current[i]);
    }

    for (let i = 0; i < PEDALS.length - 1; i++) {
      pedalOutputNodesRef.current[i].connect(pedalInputNodesRef.current[i + 1]);
    }
    pedalOutputNodesRef.current[PEDALS.length - 1].connect(audioContext.destination);
  }

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
  }

  const toggleRecording = async () => {
    if (isRecording) {
      recorderRef.current?.stop();
      sourceNodeRef.current?.disconnect();
      setIsRecording(false);
    } else {
      if (!permissionsGranted) {
        const recPerm = await AudioManager.requestRecordingPermissions();
        permissionsGranted = recPerm === 'Granted';
      }
      if (permissionsGranted) {
        const recorder = new AudioRecorder();
        const adapter = audioContext.createRecorderAdapter();
        recorder.connect(adapter);
        recorderRef.current = recorder;
        sourceNodeRef.current = adapter;
        sourceNodeRef.current.connect(pedalInputNodesRef.current[0]);
        if (audioContext.state === 'suspended') {
          audioContext.resume();
        }
        recorder.start();
        setIsRecording(true);
      } else {
        Alert.alert(
          'Insufficient permissions!',
          'You need to grant audio recording permissions to use this feature.'
        );
      };
    }
  }


  return (
    <Container centered>
      {isLoading ? (
        <ActivityIndicator color="#FFFFFF" />
      ) : (
        <>
          <ScrollView>
          {PEDALS.map((pedal, index) => {
              const PedalComponent = pedal.component;
              return (
                <View key={pedal.id} style={styles.container}>
                  <PedalComponent
                    context={audioContext}
                    inputNode={pedalInputNodesRef.current[index]}
                    outputNode={pedalOutputNodesRef.current[index]}
                  />
                </View>
              );
            })}
          </ScrollView>
          <View style={styles.controls}>
            <View style= {{ display: isRecording ? 'none' : 'flex' }}>
            <Button
              title={isPlaying ? 'Stop' : 'Play'}
              onPress={togglePlayback}
            />
            </View>

            <View style= {{ display: isPlaying ? 'none' : 'flex' }}>
            <Button
              title={isRecording ? 'Stop' : 'Record'}
              onPress={toggleRecording}
            />
            </View>
          </View>
        </>
      )}
    </Container>
  );
}

const styles = StyleSheet.create({
  container: {
    width: screenWdith * 0.9,
    alignItems: 'center',
    gap: 20,
  },
  controls: {
    display: 'flex',
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 20,
    marginTop: 20,
    width: 200,
  },
});
