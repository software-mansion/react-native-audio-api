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
  const [isLoading, setIsLoading] = useState(false);

  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);

  const [startTime, setStartTime] = useState(0);
  const [offset, setOffset] = useState(0);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);

  const handleSetAudioSourceFromFile = async () => {
    try {
      const result = await DocumentPicker.getDocumentAsync({
        type: 'audio/*',
        multiple: false,
      });

      if (result.canceled === false) {
        bufferSourceRef.current?.stop();
        setIsPlaying(false);

        setIsLoading(true);
        await fetchAudioBuffer(result.assets[0].uri);
        setIsLoading(false);
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
    if (!audioBuffer || !audioContextRef.current) {
      return;
    }

    if (isPlaying) {
      const stopTime = audioContextRef.current.currentTime;
      bufferSourceRef.current?.stop(stopTime);
      setOffset((prev) => prev + stopTime - startTime);
    } else {
      bufferSourceRef.current = audioContextRef.current.createBufferSource();
      bufferSourceRef.current.buffer = audioBuffer;
      bufferSourceRef.current.connect(audioContextRef.current.destination);

      setStartTime(audioContextRef.current.currentTime);
      bufferSourceRef.current.start(startTime, offset);
    }

    setIsPlaying((prev) => !prev);
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
      <Button
        title="Set audio source from file"
        onPress={handleSetAudioSourceFromFile}
        width={200}
      />
      {isLoading && <ActivityIndicator color="#FFFFFF" />}
      <Spacer.Vertical size={20} />
      <Button
        title={isPlaying ? 'Stop' : 'Play'}
        onPress={handlePress}
        disabled={!audioBuffer}
      />
    </Container>
  );
};

export default AudioFile;
