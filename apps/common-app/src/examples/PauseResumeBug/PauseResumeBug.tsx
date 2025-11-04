import React, { useState, useEffect, FC, useRef } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import { AudioContext, AudioBuffer } from 'react-native-audio-api';

import { Container, Button, Spacer } from '../../components';
import { colors } from '../../styles';

const AUDIO_URL =
  'https://software-mansion.github.io/react-native-audio-api/audio/voice/example-voice-01.mp3';

const PauseResumeBug: FC = () => {
  const [status, setStatus] = useState('Ready');
  const [isPlaying, setIsPlaying] = useState(false);
  const [isPaused, setIsPaused] = useState(false);
  const [isContextSuspended, setIsContextSuspended] = useState(false);

  const audioContextRef = useRef<AudioContext | null>(null);
  const queueSourceRef = useRef<any>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);

  const initializeAudio = async () => {
    audioContextRef.current = new AudioContext();
    const audioContext = audioContextRef.current;

    const response = await fetch(AUDIO_URL);
    const arrayBuffer = await response.arrayBuffer();
    audioBufferRef.current = await audioContext.decodeAudioData(arrayBuffer);

    queueSourceRef.current = audioContext.createBufferQueueSource({
      pitchCorrection: false,
    });
    queueSourceRef.current.connect(audioContext.destination);
    queueSourceRef.current.enqueueBuffer(audioBufferRef.current);

    setStatus('Initialized');
  };

  const testBugScenario = async () => {
    if (!queueSourceRef.current || !audioContextRef.current) return;

    // Start playback
    queueSourceRef.current.start(0);
    setIsPlaying(true);
    setStatus('Playing');

    // // Wait 3 seconds
    await new Promise((resolve) => setTimeout(resolve, 3000));

    // // Pause playback
    queueSourceRef.current.pause();
    setIsPaused(true);
    setIsPlaying(false);
    setStatus('Paused');
    // await audioContextRef.current.suspend();
    // setIsContextSuspended(true);
    // setStatus('Context Suspended');

    // // Resume context
    // await audioContextRef.current.resume();
    // setIsContextSuspended(false);
    // setStatus('Context Resumed');
    const now = audioContextRef.current.currentTime;
    queueSourceRef.current.start(now + 3);
    setIsPlaying(true);
    setIsPaused(false);
    setStatus('Started after context resume');
  };

  const cleanup = () => {
    if (queueSourceRef.current) {
      queueSourceRef.current.disconnect();
    }
    if (audioContextRef.current) {
      audioContextRef.current.close();
    }
    setIsPlaying(false);
    setIsPaused(false);
    setIsContextSuspended(false);
    setStatus('Cleaned up');
  };

  useEffect(() => {
    return cleanup;
  }, []);

  return (
    <Container>
      <Text style={styles.title}>Pause/Resume Bug</Text>
      <Text style={styles.subtitle}>
        Start → (3s) → Pause → Suspend → Resume → Start
      </Text>

      <Spacer.Vertical size={20} />

      <View style={styles.statusContainer}>
        <Text style={styles.statusText}>Status: {status}</Text>
        <Text style={styles.statusText}>
          Playing: {isPlaying ? 'Yes' : 'No'}
        </Text>
        <Text style={styles.statusText}>Paused: {isPaused ? 'Yes' : 'No'}</Text>
        <Text style={styles.statusText}>
          Context Suspended: {isContextSuspended ? 'Yes' : 'No'}
        </Text>
      </View>

      <Spacer.Vertical size={20} />

      <View style={styles.buttonContainer}>
        <Button
          onPress={initializeAudio}
          title="Initialize Audio"
          disabled={!!audioBufferRef.current}
        />
      </View>

      <Spacer.Vertical size={15} />

      <View style={styles.buttonContainer}>
        <Button
          onPress={testBugScenario}
          title="Test Bug Scenario"
          disabled={!audioBufferRef.current}
        />
      </View>

      <Spacer.Vertical size={15} />

      <View style={styles.buttonContainer}>
        <Button onPress={cleanup} title="Cleanup" />
      </View>

      <Spacer.Vertical size={20} />
    </Container>
  );
};

const styles = StyleSheet.create({
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: colors.white,
    textAlign: 'center',
  },
  subtitle: {
    fontSize: 16,
    color: colors.white,
    textAlign: 'center',
    opacity: 0.8,
    marginBottom: 20,
  },
  statusContainer: {
    backgroundColor: 'rgba(255, 255, 255, 0.1)',
    padding: 15,
    borderRadius: 8,
    marginBottom: 20,
  },
  statusText: {
    fontSize: 14,
    color: colors.white,
    marginBottom: 5,
  },
  buttonContainer: {
    flexDirection: 'row',
    justifyContent: 'center',
    alignItems: 'center',
    marginBottom: 10,
  },
  instructions: {
    backgroundColor: 'rgba(52, 152, 219, 0.1)',
    padding: 15,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: 'rgba(52, 152, 219, 0.3)',
  },
  instructionTitle: {
    fontSize: 16,
    fontWeight: '600',
    color: colors.white,
    marginBottom: 10,
  },
  instructionText: {
    fontSize: 14,
    color: colors.white,
    marginBottom: 5,
    lineHeight: 20,
  },
});

export default PauseResumeBug;

/*
this	audioapi::AudioBufferQueueSourceNode *	0x11ed46318	0x000000011ed46318
*/