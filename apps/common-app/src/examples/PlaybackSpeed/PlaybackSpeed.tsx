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
import { AudioContext } from 'react-native-audio-api';
import type { AudioBuffer, AudioBufferSourceNode } from 'react-native-audio-api';
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
  const bufferRef = useRef<AudioBuffer | null>(null);
  const playbackSpeedRef = useRef<number>(playbackSpeed);
  const playbackAnchorRef = useRef<{
    contextTime: number;
    offset: number;
  } | null>(null);

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

  const loadBuffer = useCallback(async (): Promise<AudioBuffer> => {
    if (bufferRef.current) {
      return bufferRef.current;
    }

    const audioContext = initializeAudioContext();
    const buffer = await audioContext.decodePCMInBase64(PCM_DATA, 48000, 1, true);
    bufferRef.current = buffer;
    return buffer;
  }, [initializeAudioContext]);

  const getCurrentOffset = useCallback((): number => {
    const audioContext = aCtxRef.current;
    const buffer = bufferRef.current;
    const anchor = playbackAnchorRef.current;

    if (!audioContext || !buffer || !anchor || buffer.duration <= 0) {
      return 0;
    }

    const elapsed = audioContext.currentTime - anchor.contextTime;
    const advanced = anchor.offset + elapsed * playbackSpeedRef.current;
    const wrapped = advanced % buffer.duration;

    return wrapped < 0 ? wrapped + buffer.duration : wrapped;
  }, []);

  const createSource = useCallback(
    async (pitchCorrection: boolean): Promise<AudioBufferSourceNode> => {
      const audioContext = initializeAudioContext();

      setIsLoading(true);

      try {
        const buffer = await loadBuffer();

        const source = audioContext.createBufferSource({
          pitchCorrection,
        });

        source.buffer = buffer;
        source.playbackRate.value = playbackSpeedRef.current;
        source.loop = true;

        return source;
      } catch (error) {
        console.error('Failed to create audio source:', error);
        throw error;
      } finally {
        setIsLoading(false);
      }
    },
    [initializeAudioContext, loadBuffer]
  );

  const stopPlayback = useCallback(() => {
    if (sourceRef.current) {
      sourceRef.current.onEnded = null;
      sourceRef.current.stop();
      sourceRef.current = null;
    }
    playbackAnchorRef.current = null;
    setIsPlaying(false);
  }, []);

  const startPlayback = useCallback(
    async (pitchCorrection: boolean, startOffset: number = 0) => {
      try {
        const audioContext = initializeAudioContext();
        const source = await createSource(pitchCorrection);

        sourceRef.current = source;

        sourceRef.current.onEnded = () => {
          setIsPlaying(false);
          sourceRef.current = null;
          playbackAnchorRef.current = null;
        };

        sourceRef.current.connect(audioContext.destination);

        const startTime = audioContext.currentTime;
        sourceRef.current.start(startTime, startOffset);
        playbackAnchorRef.current = {
          contextTime: startTime,
          offset: startOffset,
        };

        setIsPlaying(true);
      } catch (error) {
        console.error('Failed to start playback:', error);
        setIsPlaying(false);
      }
    },
    [createSource, initializeAudioContext]
  );

  const togglePlayPause = useCallback(async () => {
    if (isPlaying) {
      stopPlayback();
    } else {
      await startPlayback(audioSettings.pitchCorrection);
    }
  }, [isPlaying, stopPlayback, startPlayback, audioSettings]);

  const handlePlaybackSpeedChange = useCallback(
    (newSpeed: number) => {
      const audioContext = aCtxRef.current;

      if (sourceRef.current && playbackAnchorRef.current && audioContext) {
        playbackAnchorRef.current = {
          contextTime: audioContext.currentTime,
          offset: getCurrentOffset(),
        };
        sourceRef.current.playbackRate.value = newSpeed;
      }

      playbackSpeedRef.current = newSpeed;
      setPlaybackSpeed(newSpeed);
    },
    [getCurrentOffset]
  );

  const handleTimeStretchingAlgorithmChange = useCallback(
    (newMode: TimeStretchingAlgorithm) => {
      const wasPlaying = isPlaying;
      const resumeOffset = wasPlaying ? getCurrentOffset() : 0;

      setTimeStretchingAlgorithm(newMode);

      if (wasPlaying) {
        stopPlayback();
        const { pitchCorrection } = getAudioSettings(newMode);
        startPlayback(pitchCorrection, resumeOffset);
      }
    },
    [isPlaying, stopPlayback, startPlayback, getCurrentOffset]
  );

  useEffect(() => {
    return () => {
      stopPlayback();
      bufferRef.current = null;
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
