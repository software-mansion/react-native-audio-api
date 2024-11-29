import React from 'react';
import { useState, useRef, useEffect, FC } from 'react';
import { Container, Button } from '../../components';

import {
  AudioContext,
  AudioBufferSourceNode,
  AudioBuffer,
} from 'react-native-audio-api';

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
    audioBufferSourceNodeRef.current.buffer;
    audioBufferSourceNodeRef.current.connect(
      audioContextRef.current.destination
    );
  };

  const fetchAudioBuffer = async () => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    setAudioBuffer(
      audioContextRef.current.decodeAudioDataSource(
        '/Users/maciejmakowski/projects/react-native-audio-api/apps/common-app/src/examples/AudioFile/runaway_kanye_west.mp3'
      )
    );
  };

  const handlePress = () => {
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

    fetchAudioBuffer();

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  return (
    <Container centered>
      <Button title={isPlaying ? 'Stop' : 'Play'} onPress={handlePress} />
    </Container>
  );
};

export default AudioFile;
