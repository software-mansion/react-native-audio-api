import React, { useState, useEffect, useRef } from 'react';
import * as FileSystem from 'expo-file-system';
import {
  AudioContext,
  AudioBuffer,
  AudioBufferSourceNode,
} from 'react-native-audio-api';
import { ActivityIndicator, View, Button } from 'react-native';

const AudioVisualizer: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);

  const handlePlayPause = () => {
    if (isPlaying) {
      bufferSourceRef.current?.stop();
    } else {
      if (!audioContextRef.current) {
        return
      }

      bufferSourceRef.current = audioContextRef.current.createBufferSource();
      bufferSourceRef.current.buffer = audioBufferRef.current;
      bufferSourceRef.current.connect(audioContextRef.current.destination);

      bufferSourceRef.current.start();
    }

    setIsPlaying((prev) => !prev);
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    const fetchBuffer = async () => {
      setIsLoading(true);
      audioBufferRef.current = await FileSystem.downloadAsync(
        'https://software-mansion-labs.github.io/react-native-audio-api/audio/music/example-music-02.mp3',
        FileSystem.documentDirectory + 'example-see-your-sound.mp3'
      ).then(({ uri }) => {
        return audioContextRef.current!.decodeAudioDataSource(uri);
      });

      setIsLoading(false);
    };

    fetchBuffer();

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  return (
      <View
        style={{ flex: 0.5, justifyContent: 'center', alignItems: 'center' }}>
        {isLoading && <ActivityIndicator color="#FFFFFF" />}
        <View style={{
            justifyContent: 'center',
            flexDirection: 'row',
            marginTop: 16,
          }}>
        <Button
            onPress={handlePlayPause}
            title={isPlaying ? 'Pause' : 'Play'}
            disabled={!audioBufferRef.current}
          />
        </View>
      </View>
  );
};

export default AudioVisualizer;
