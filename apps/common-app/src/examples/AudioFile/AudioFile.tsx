import React, { useCallback, useEffect, useRef, useState, FC } from 'react';
import * as FileSystem from 'expo-file-system';
import { ActivityIndicator } from 'react-native';
import {
  AudioControls,
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
} from 'react-native-audio-api';

import { Container, Button, Spacer } from '../../components';

const URL =
  'https://software-mansion-labs.github.io/react-native-audio-api/audio/music/example-music-04.mp3';

const LOOP_START = 1;
const LOOP_END = 2;

const AudioFile: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);

  const [startTime, setStartTime] = useState(0);
  const [offset, setOffset] = useState(0);

  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);

  const fetchAudioBuffer = useCallback(async () => {
    setIsLoading(true);
    const buffer = await FileSystem.downloadAsync(
      URL,
      FileSystem.documentDirectory + 'audio.mp3'
    )
      .then(({ uri }) => {
        return audioContextRef.current!.decodeAudioDataSource(uri);
      })
      .catch((error) => {
        console.error('Error decoding audio data source:', error);
        return null;
      });

    setAudioBuffer(buffer);

    setIsLoading(false);
  }, []);

  const handlePress = () => {
    if (!audioContextRef.current) {
      return;
    }

    AudioControls.init();
    AudioControls.showNowPlayingInfo({
      title: 'Example Music',
      artist: 'Software Mansion',
      artwork:
        'https://software-mansion-labs.github.io/react-native-audio-api/audio/music/example-music-04.jpg',
      duration: 10,
    });

    AudioControls.addEventListener('stop', () => {});

    // if (isPlaying) {
    //   const stopTime = audioContextRef.current.currentTime;

    //   bufferSourceRef.current?.stop(stopTime);
    //   setOffset((prev) => prev + stopTime - startTime);
    // } else {
    //   if (!audioBuffer) {
    //     fetchAudioBuffer();
    //   }

    //   bufferSourceRef.current = audioContextRef.current.createBufferSource();
    //   bufferSourceRef.current.buffer = audioBuffer;
    //   bufferSourceRef.current.loop = true;
    //   bufferSourceRef.current.loopStart = LOOP_START;
    //   bufferSourceRef.current.loopEnd = LOOP_END;
    //   bufferSourceRef.current.connect(audioContextRef.current.destination);

    //   setStartTime(audioContextRef.current.currentTime);
    //   bufferSourceRef.current.start(startTime, offset);
    // }

    // setIsPlaying((prev) => !prev);
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    fetchAudioBuffer();

    return () => {
      audioContextRef.current?.close();
    };
  }, [fetchAudioBuffer]);

  return (
    <Container centered>
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
