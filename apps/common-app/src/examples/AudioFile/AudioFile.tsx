import React, { useCallback, useEffect, useRef, useState, FC } from 'react';
import { Container, Button, Spacer } from '../../components';
import * as DocumentPicker from 'expo-document-picker';

import {
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
} from 'react-native-audio-api';
import { ActivityIndicator } from 'react-native';

const AudioFile: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);

  const audioContextRef = useRef<AudioContext | null>(null);
  const audioBufferSourceNodeRef = useRef<AudioBufferSourceNode | null>(null);
  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);

  const setup = () => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    audioBufferSourceNodeRef.current =
      audioContextRef.current.createBufferSource();

    audioBufferSourceNodeRef.current.connect(
      audioContextRef.current.destination
    );
  };

  const handleSetAudioSourceFromFile = async () => {
    try {
      const result = await DocumentPicker.getDocumentAsync({
        type: 'audio/*',
        multiple: false,
      });
      if (result.canceled === false) {
        audioBufferSourceNodeRef.current?.stop();
        setIsPlaying(false);
        setAudioBuffer(null);

        await fetchAudioBuffer(result.assets[0].uri.replace('file://', ''));
      }
    } catch (error) {
      console.error('Error picking file:', error);
    }
  };

  const fetchAudioBuffer = useCallback(async (assetUri: string) => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    const buffer =
      await audioContextRef.current.decodeAudioDataSource(assetUri);

    setAudioBuffer(buffer);
  }, []);

  const handlePress = () => {
    if (!audioBuffer) {
      return;
    }

    if (isPlaying) {
      audioBufferSourceNodeRef.current?.stop();
    } else {
      setup();
      audioBufferSourceNodeRef.current!.buffer = audioBuffer;
      audioBufferSourceNodeRef.current?.start();
    }

    setIsPlaying(!isPlaying);
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    return () => {
      audioContextRef.current?.close();
    };
  }, [fetchAudioBuffer]);

  return (
    <Container centered>
      <Button title={isPlaying ? 'Stop' : 'Play'} onPress={handlePress} />
      {!audioBuffer && <ActivityIndicator color="#FFFFFF" />}
      <Spacer.Vertical size={20} />
      <Button
        title="Set audio source from file"
        onPress={handleSetAudioSourceFromFile}
      />
    </Container>
  );
};

export default AudioFile;
