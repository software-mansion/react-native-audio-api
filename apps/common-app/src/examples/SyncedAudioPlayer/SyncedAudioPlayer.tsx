import React, { useEffect, useRef, useState } from 'react';
import { Container, Button, Spacer, Slider } from '../../components';
import {
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
  GainNode,
} from 'react-native-audio-api';
import { ActivityIndicator } from 'react-native';

type SoundTracks = Record<
  string,
  {
    audioBuffer?: AudioBuffer;
    audioBufferNode?: AudioBufferSourceNode;
    gainNode: GainNode;
    volume: number;
  }
>;

const SyncedAudioPlayer = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(true);
  const isInitialized = useRef(false);

  const [speed, setSpeed] = useState(1);

  const audioContextRef = useRef<AudioContext | null>(null);
  const soundTracks = useRef<SoundTracks>({});

  const loadAudioFiles = async () => {
    try {
      if (!audioContextRef.current) {
        audioContextRef.current = new AudioContext();
      }

      const audioFiles = [
        'https://bachatapp.gerwim.com/bachata/bass.mp3',
        'https://bachatapp.gerwim.com/bachata/conga.mp3',
        'https://bachatapp.gerwim.com/bachata/guira.mp3',
        'https://bachatapp.gerwim.com/bachata/kick.mp3',
        'https://bachatapp.gerwim.com/bachata/piano.mp3',
        'https://bachatapp.gerwim.com/bachata/tambora.mp3',
        'https://bachatapp.gerwim.com/bachata/trompetas.mp3',
      ];

      await Promise.all(
        audioFiles.map(async (file) => {
          try {
            const response = await fetch(file);
            const arrayBuffer = await response.arrayBuffer();

            const buffer =
              await audioContextRef.current!.decodeAudioData(arrayBuffer);
            const gainNode = audioContextRef.current!.createGain();
            gainNode.connect(audioContextRef.current!.destination);

            soundTracks.current = {
              ...soundTracks.current,
              [file]: { audioBuffer: buffer, gainNode, volume: 1 },
            };
          } catch (error) {
            console.error('Error loading file: ', file, error);
          }
        })
      );

      setIsLoading(false);
    } catch (error) {
      console.error('Error loading files:', error);
    }
  };

  useEffect(() => {
    Object.values(soundTracks.current).forEach((sound) => {
      if (!sound.audioBufferNode) return;

      sound.audioBufferNode.playbackRate.value = speed;
    });
  }, [speed]);

  const stop = () => {
    if (audioContextRef.current) audioContextRef.current.suspend();
  };

  const play = () => {
    if (!audioContextRef.current) return;

    if (isInitialized.current) {
      audioContextRef.current.resume();
      return;
    }

    Object.keys(soundTracks.current).forEach((sound) => {
      const audioNodes = soundTracks.current[sound];
      if (!audioNodes.audioBuffer) return;

      const bufferNode = audioContextRef.current.createBufferSource({
        pitchCorrection: true,
      });
      bufferNode.connect(audioNodes.gainNode);

      bufferNode.buffer = audioNodes.audioBuffer!;
      bufferNode.loop = true;
      bufferNode.playbackRate.value = speed;
      bufferNode.start();

      soundTracks.current = {
        ...soundTracks.current,
        [sound]: {
          ...audioNodes,
          audioBufferNode: bufferNode,
        },
      };
    });

    isInitialized.current = true;
  };

  const handlePress = () => {
    if (!audioContextRef.current) {
      return;
    }

    if (isPlaying) {
      stop();
    } else {
      play();
    }

    setIsPlaying((prev) => !prev);
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    loadAudioFiles();

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  return (
    <Container centered>
      {isLoading && <ActivityIndicator color="#FFFFFF" />}
      <Spacer.Vertical size={20} />
      <Button
        title={isPlaying ? 'Stop' : 'Play'}
        onPress={handlePress}
        disabled={isLoading}
      />
      <Spacer.Vertical size={20} />
      <Slider
        label="Speed"
        value={speed}
        onValueChange={setSpeed}
        min={0.7}
        max={1.3}
        step={0.01}
        minLabelWidth={80}
      />
    </Container>
  );
};

export default SyncedAudioPlayer;
