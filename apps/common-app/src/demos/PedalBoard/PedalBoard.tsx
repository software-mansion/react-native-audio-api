import React, { useEffect, useRef, useState } from 'react';
import {
  AudioBufferSourceNode,
  AudioBuffer,
  GainNode,
} from 'react-native-audio-api';
import { Container } from '../../components';
import { audioContext } from '../../singletons';
import { ActivityIndicator, View, Button, StyleSheet, ScrollView, Dimensions } from 'react-native';
import OverdrivePedal from './OverdrivePedal';
import ReverbPedal from './ReverbPedal';
import EchoPedal from './EchoPedal';

const screenWdith = Dimensions.get('window').width;

const URL = 'http://localhost:3000/react-native-audio-api/audio/music/105.wav';

export default function PedalBoard() {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);

  const sourceNodeRef = useRef<AudioBufferSourceNode>(null);
  const pedalInputNodesRef = useRef<GainNode[]>([]);
  const pedalOutputNodesRef = useRef<GainNode[]>([]);

  useEffect(() => {
    const init = async () => {
      setIsLoading(true);

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

        for (let i = 0; i < 3; i++) {
          const input = audioContext.createGain();
          const output = audioContext.createGain();
          pedalInputNodesRef.current.push(input);
          pedalOutputNodesRef.current.push(output);
        }
        setAudioBuffer(audioBuffer);
      } catch (error) {
        console.error('Error loading audio:', error);
      } finally {
        setIsLoading(false);
      }
    };

    init();
    handleConnections();

    return () => {
      sourceNodeRef.current?.disconnect();
      sourceNodeRef.current?.stop();
    };
  }, []);

  const handleConnections = () => {
    if (!sourceNodeRef.current) {
      return;
    }
    const sourceNode = sourceNodeRef.current;
    sourceNode.connect(pedalInputNodesRef.current[0]);
    pedalInputNodesRef.current[0].connect(pedalOutputNodesRef.current[0]);
    pedalOutputNodesRef.current[0].connect(pedalInputNodesRef.current[1]);
    pedalInputNodesRef.current[1].connect(pedalOutputNodesRef.current[1]);
    pedalOutputNodesRef.current[1].connect(pedalInputNodesRef.current[2]);
    pedalInputNodesRef.current[2].connect(pedalOutputNodesRef.current[2]);
    pedalOutputNodesRef.current[2].connect(audioContext.destination);
  }

  const togglePlayback = () => {
    if (isPlaying) {
      sourceNodeRef.current?.stop();
      setIsPlaying(false);
    } else {
      if (!audioBuffer) {
        return;
      }
      sourceNodeRef.current = audioContext.createBufferSource();
      sourceNodeRef.current.buffer = audioBuffer;
      handleConnections();
      sourceNodeRef.current.start();
      setIsPlaying(true);
    }
  }


  return (
    <Container centered>
      {isLoading ? (
        <ActivityIndicator color="#FFFFFF" />
      ) : (
        <>
          <ScrollView>
          <View style={styles.container}>
            <OverdrivePedal context={audioContext} inputNode={pedalInputNodesRef.current[0]} outputNode={pedalOutputNodesRef.current[0]}/>
          </View>
          <View style={styles.container}>
            <ReverbPedal context={audioContext} inputNode={pedalInputNodesRef.current[1]} outputNode={pedalOutputNodesRef.current[1]}/>
          </View>
          <View style={styles.container}>
            <EchoPedal context={audioContext} inputNode={pedalInputNodesRef.current[2]} outputNode={pedalOutputNodesRef.current[2]}/>
          </View>
          </ScrollView>
          <View style={styles.controls}>
            <Button
              title={isPlaying ? 'Stop' : 'Play'}
              onPress={togglePlayback}
            />
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
    marginTop: 20,
    width: 200,
  },
});
