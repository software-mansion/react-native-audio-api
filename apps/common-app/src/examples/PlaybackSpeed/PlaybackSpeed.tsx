import React, {
  useState,
  FC,
  useRef,
  useEffect,
  useCallback,
  useMemo,
} from 'react';
import { View, StyleSheet } from 'react-native';
import { Container, Button, Spacer, Slider, Select } from '../../components';
import { AudioContext, changePlaybackSpeed } from 'react-native-audio-api';
import type { AudioBufferSourceNode } from 'react-native-audio-api';
import {
  PCM_DATA,
  labelWidth,
  PLAYBACK_SPEED_CONFIG,
  SAMPLE_RATE,
} from './constants';
import { TimeStretchingAlgorithm, TIME_STRETCHING_OPTIONS } from './types';
import { getAudioSettings } from './helpers';

const PlaybackSpeed: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [playbackSpeed, setPlaybackSpeed] = useState<number>(
    PLAYBACK_SPEED_CONFIG.default
  );
  const [timeStretchingAlgorithm, setTimeStretchingAlgorithm] =
    useState<TimeStretchingAlgorithm>('linear');

  const aCtxRef = useRef<AudioContext | null>(null);
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);

  const audioSettings = useMemo(
    () => getAudioSettings(timeStretchingAlgorithm),
    [timeStretchingAlgorithm]
  );

  const initializeAudioContext = useCallback(() => {
    if (!aCtxRef.current) {
      aCtxRef.current = new AudioContext({ sampleRate: SAMPLE_RATE });
    }
    return aCtxRef.current;
  }, []);

  const createSource = useCallback(async (): Promise<AudioBufferSourceNode> => {
    const audioContext = initializeAudioContext();

    setIsLoading(true);

    try {
      const buffer = await audioContext
        .decodePCMInBase64(PCM_DATA, 48000, 1, true)
        .then((audioBuffer) =>
          changePlaybackSpeed(
            audioBuffer,
            audioSettings.PSOLA ? playbackSpeed : 1
          )
        );

      const source = audioContext.createBufferSource(audioSettings.PSOLA ? { pitchCorrection: false } : { pitchCorrection: audioSettings.pitchCorrection });

      source.buffer = buffer;
      source.playbackRate.value = audioSettings.PSOLA ? 1 : playbackSpeed;
      source.loop = true;

      return source;
    } catch (error) {
      console.error('Failed to create audio source:', error);
      throw error;
    } finally {
      setIsLoading(false);
    }
  }, [audioSettings, playbackSpeed, initializeAudioContext]);

  const stopPlayback = useCallback(() => {
    if (sourceRef.current) {
      sourceRef.current.onEnded = null;
      sourceRef.current.stop();
      sourceRef.current = null;
    }
    setIsPlaying(false);
  }, []);

  const startPlayback = useCallback(async () => {
    try {
      const audioContext = initializeAudioContext();
      const source = await createSource();

      sourceRef.current = source;

      sourceRef.current.onEnded = () => {
        setIsPlaying(false);
        sourceRef.current = null;
      };

      sourceRef.current.connect(audioContext.destination);
      sourceRef.current.start(audioContext.currentTime);

      setIsPlaying(true);
    } catch (error) {
      console.error('Failed to start playback:', error);
      setIsPlaying(false);
    }
  }, [createSource, initializeAudioContext]);

  const togglePlayPause = useCallback(async () => {
    if (isPlaying) {
      stopPlayback();
    } else {
      await startPlayback();
    }
  }, [isPlaying, stopPlayback, startPlayback]);

  const handlePlaybackSpeedChange = useCallback(
    (newSpeed: number) => {
      if (audioSettings.PSOLA) {
        stopPlayback();
      } else {
        if (aCtxRef.current && sourceRef.current) {
          sourceRef.current.playbackRate.value = newSpeed;
        }
      }

      setPlaybackSpeed(newSpeed);
    },
    [audioSettings, stopPlayback]
  );

  const handleTimeStretchingAlgorithmChange = useCallback(
    (newMode: TimeStretchingAlgorithm) => {
      setTimeStretchingAlgorithm(newMode);
      if (isPlaying) {
        stopPlayback();
      }
    },
    [isPlaying, stopPlayback]
  );

  useEffect(() => {
    return () => {
      stopPlayback();
      aCtxRef.current?.close();
      aCtxRef.current = null;
    };
  }, [stopPlayback]);

  return (
    <Container>
      <View style={styles.algorithmSelectContainer}>
        <Select
          value={timeStretchingAlgorithm}
          options={TIME_STRETCHING_OPTIONS}
          onChange={handleTimeStretchingAlgorithmChange}
        />
      </View>

      <View style={styles.controlsContainer}>
        <Button
          title={isPlaying ? 'Stop' : 'Play'}
          onPress={togglePlayPause}
          disabled={isLoading}
        />

        <Spacer.Vertical size={20} />

        <Slider
          label="Playback Speed"
          value={playbackSpeed}
          onValueChange={handlePlaybackSpeedChange}
          min={PLAYBACK_SPEED_CONFIG.min}
          max={PLAYBACK_SPEED_CONFIG.max}
          step={PLAYBACK_SPEED_CONFIG.step}
          minLabelWidth={labelWidth}
        />
      </View>
    </Container>
  );
};

const styles = StyleSheet.create({
  algorithmSelectContainer: {
    paddingTop: 20,
    paddingHorizontal: 20,
  },
  controlsContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
});

export default PlaybackSpeed;
