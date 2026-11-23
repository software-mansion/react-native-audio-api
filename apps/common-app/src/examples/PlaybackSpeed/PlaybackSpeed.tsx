import React, { FC, useEffect, useRef, useState } from 'react';
import { StyleSheet, View } from 'react-native';
import type {
  AudioBuffer,
  AudioBufferSourceNode,
} from 'react-native-audio-api';

import { Button, Container, Select, Slider } from '../../components';
import { useAudioContext } from '../../utils/audio';
import { PCM_DATA, PLAYBACK_SPEED_CONFIG, SAMPLE_RATE } from './constants';

type TimeStretchingAlgorithm = 'linear' | 'pitchCorrection';

const TIME_STRETCHING_OPTIONS: TimeStretchingAlgorithm[] = [
  'linear',
  'pitchCorrection',
];

const PlaybackSpeed: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [playbackSpeed, setPlaybackSpeed] = useState<number>(
    PLAYBACK_SPEED_CONFIG.default
  );
  const [timeStretchingAlgorithm, setTimeStretchingAlgorithm] =
    useState<TimeStretchingAlgorithm>('linear');

  const audioContext = useAudioContext({ sampleRate: SAMPLE_RATE });
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);
  const bufferRef = useRef<AudioBuffer | null>(null);
  const playbackSpeedRef = useRef<number>(playbackSpeed);
  const playbackAnchorRef = useRef<{
    contextTime: number;
    offset: number;
  } | null>(null);

  const loadBuffer = async (): Promise<AudioBuffer> => {
    if (!bufferRef.current) {
      bufferRef.current = await audioContext.decodePCMInBase64(
        PCM_DATA,
        48000,
        1,
        true
      );
    }
    return bufferRef.current;
  };

  const getCurrentOffset = (): number => {
    const buffer = bufferRef.current;
    const anchor = playbackAnchorRef.current;

    if (!buffer || !anchor || buffer.duration <= 0) {
      return 0;
    }

    const elapsed = audioContext.currentTime - anchor.contextTime;
    const advanced = anchor.offset + elapsed * playbackSpeedRef.current;
    const wrapped = advanced % buffer.duration;

    return wrapped < 0 ? wrapped + buffer.duration : wrapped;
  };

  const stopPlayback = () => {
    if (sourceRef.current) {
      sourceRef.current.onended = null;
      sourceRef.current.stop();
      sourceRef.current.disconnect();
      sourceRef.current = null;
    }
    playbackAnchorRef.current = null;
    setIsPlaying(false);
  };

  const startPlayback = async (
    algorithm: TimeStretchingAlgorithm,
    startOffset = 0
  ) => {
    setIsLoading(true);

    try {
      const source = audioContext.createBufferSource({
        pitchCorrection: algorithm === 'pitchCorrection',
      });
      source.buffer = await loadBuffer();
      source.playbackRate.value = playbackSpeedRef.current;
      source.loop = true;
      source.onended = () => {
        source.disconnect();
        setIsPlaying(false);
        sourceRef.current = null;
        playbackAnchorRef.current = null;
      };
      source.connect(audioContext.destination);

      const startTime = audioContext.currentTime;
      source.start(startTime, startOffset);
      sourceRef.current = source;
      playbackAnchorRef.current = {
        contextTime: startTime,
        offset: startOffset,
      };

      setIsPlaying(true);
    } catch (error) {
      console.error('Failed to start playback:', error);
      setIsPlaying(false);
    } finally {
      setIsLoading(false);
    }
  };

  const togglePlayPause = () => {
    if (isPlaying) {
      stopPlayback();
    } else {
      startPlayback(timeStretchingAlgorithm);
    }
  };

  const handlePlaybackSpeedChange = (newSpeed: number) => {
    if (sourceRef.current && playbackAnchorRef.current) {
      playbackAnchorRef.current = {
        contextTime: audioContext.currentTime,
        offset: getCurrentOffset(),
      };
      sourceRef.current.playbackRate.value = newSpeed;
    }

    playbackSpeedRef.current = newSpeed;
    setPlaybackSpeed(newSpeed);
  };

  const handleTimeStretchingAlgorithmChange = (
    algorithm: TimeStretchingAlgorithm
  ) => {
    setTimeStretchingAlgorithm(algorithm);

    if (isPlaying) {
      const resumeOffset = getCurrentOffset();
      stopPlayback();
      startPlayback(algorithm, resumeOffset);
    }
  };

  useEffect(
    () => () => {
      sourceRef.current?.disconnect();
    },
    []
  );

  return (
    <Container>
      <Select
        value={timeStretchingAlgorithm}
        options={TIME_STRETCHING_OPTIONS}
        onChange={handleTimeStretchingAlgorithmChange}
      />
      <View style={styles.controls}>
        <Button
          title={isPlaying ? 'Stop' : 'Play'}
          onPress={togglePlayPause}
          disabled={isLoading}
        />
        <Slider
          label="Playback Speed"
          value={playbackSpeed}
          onValueChange={handlePlaybackSpeedChange}
          min={PLAYBACK_SPEED_CONFIG.min}
          max={PLAYBACK_SPEED_CONFIG.max}
          step={PLAYBACK_SPEED_CONFIG.step}
        />
      </View>
    </Container>
  );
};

const styles = StyleSheet.create({
  controls: {
    flex: 1,
    justifyContent: 'center',
    gap: 20,
  },
});

export default PlaybackSpeed;
