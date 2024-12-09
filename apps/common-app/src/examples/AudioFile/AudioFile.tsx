import React, { useCallback, useEffect, useRef, useState, FC } from 'react';
import { Container, Button, Spacer } from '../../components';
import * as DocumentPicker from 'expo-document-picker';
import { View, StyleSheet } from 'react-native';

import {
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
} from 'react-native-audio-api';
import { ActivityIndicator } from 'react-native';

const sampleUrl =
  'https://audio-ssl.itunes.apple.com/apple-assets-us-std-000001/AudioPreview18/v4/9c/db/54/9cdb54b3-5c52-3063-b1ad-abe42955edb5/mzaf_520282131402737225.plus.aac.p.m4a';

const AudioFile: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);

  const audioContextRef = useRef<AudioContext | null>(null);
  const audioBufferSourceNodeRef = useRef<AudioBufferSourceNode | null>(null);
  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);

  const handleSetAudioSourceFromFile = async () => {
    try {
      const result = await DocumentPicker.getDocumentAsync({
        type: 'audio/*',
        multiple: false,
      });
      if (result.canceled === false) {
        setAudioBuffer(null);
        await fetchAudioBuffer(result.assets[0].uri.replace('file://', ''));
      }
    } catch (error) {
      console.error('Error picking file:', error);
    }
  };

  const handleSetAudioSourceFromUrl = async () => {
    setAudioBuffer(null);
    await fetchAudioBuffer(sampleUrl);
  };

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

    //fetchAudioBuffer(sampleUrl);

    return () => {
      audioContextRef.current?.close();
    };
  }, [fetchAudioBuffer]);

  return (
    <Container centered>
      <Button title={isPlaying ? 'Stop' : 'Play'} onPress={handlePress} />
      {!audioBuffer && <ActivityIndicator color="#FFFFFF" />}
      <Spacer.Vertical size={20} />
      <View style={styles.container}>
        <Button
          title="Set audio source from file"
          onPress={handleSetAudioSourceFromFile}
        />
        <Spacer.Horizontal size={10} />
        <Button
          title="Set audio source from url"
          onPress={handleSetAudioSourceFromUrl}
        />
      </View>
    </Container>
  );
};

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    justifyContent: 'center',
    alignItems: 'center',
  },
});

export default AudioFile;
