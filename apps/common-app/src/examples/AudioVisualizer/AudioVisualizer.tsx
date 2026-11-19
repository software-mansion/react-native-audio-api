import React, { useEffect, useRef, useState } from 'react';
import {
  ActivityIndicator,
  Pressable,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
} from 'react-native-audio-api';
import { WorkletNode } from 'react-native-audio-worklets';
import { useSharedValue } from 'react-native-reanimated';

import {
  AppIcon,
  BottomAccessory,
  Container,
  useBottomAccessorySpace,
} from '../../components';
import { colors } from '../../styles';
import FreqTimeChart from './FreqTimeChart';

const FFT_SIZE = 512;
const FREQUENCY_BIN_COUNT = FFT_SIZE / 2;

const URL =
  'https://docs.swmansion.com/react-native-audio-api/audio/music/example-music-02.mp3';

/** How often the elapsed time and progress line refresh while playing. */
const PROGRESS_INTERVAL_MS = 250;

const ANALYSER_MIN_DB = -100;
const ANALYSER_MAX_DB = -30;

function formatTime(seconds: number) {
  const whole = Math.floor(seconds);
  return `${Math.floor(whole / 60)}:${String(whole % 60).padStart(2, '0')}`;
}

function linearMagnitudeToByte(linear: number) {
  'worklet';

  const db = linear > 0 ? 20 * Math.log10(linear) : ANALYSER_MIN_DB;
  const normalized = Math.max(
    0,
    Math.min(1, (db - ANALYSER_MIN_DB) / (ANALYSER_MAX_DB - ANALYSER_MIN_DB))
  );

  return Math.round(normalized * 255);
}

const AudioVisualizer: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [audioBuffer, setAudioBuffer] = useState<AudioBuffer | null>(null);
  const [visualizerReady, setVisualizerReady] = useState(false);

  const [startTime, setStartTime] = useState(0);
  const [offset, setOffset] = useState(0);
  /** Seconds into the track, refreshed while playing. */
  const [position, setPosition] = useState(0);
  const accessorySpace = useBottomAccessorySpace();

  const audioContextRef = useRef<AudioContext | null>(null);
  const timeWorkletRef = useRef<WorkletNode | null>(null);
  const frequencyWorkletRef = useRef<WorkletNode | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);

  const timeDataSV = useSharedValue(new Uint8Array(FFT_SIZE).fill(127));
  const frequencyDataSV = useSharedValue(
    new Uint8Array(FREQUENCY_BIN_COUNT).fill(0)
  );
  const timeDataTickSV = useSharedValue(0);
  const frequencyDataTickSV = useSharedValue(0);

  const handlePlayPause = async () => {
    if (isPlaying) {
      const stopTime = audioContextRef.current!.currentTime;
      audioContextRef.current?.suspend();
      bufferSourceRef.current?.stop(stopTime);
      const pausedAt = offset + stopTime - startTime;
      setOffset(pausedAt);
      setPosition(pausedAt);
      setIsPlaying(false);
      return;
    }

    const ctx = audioContextRef.current;
    const timeWorklet = timeWorkletRef.current;
    const frequencyWorklet = frequencyWorkletRef.current;

    if (!ctx || !timeWorklet || !frequencyWorklet || !audioBuffer) {
      return;
    }

    await ctx.resume();

    bufferSourceRef.current = ctx.createBufferSource();
    bufferSourceRef.current.buffer = audioBuffer;
    bufferSourceRef.current.connect(timeWorklet);

    const when = ctx.currentTime;
    setStartTime(when);
    bufferSourceRef.current.start(when, offset);
    setIsPlaying(true);
  };

  useEffect(() => {
    const ctx = audioContextRef.current;
    if (!isPlaying || !ctx || !audioBuffer) {
      return;
    }

    const interval = setInterval(() => {
      const current = offset + ctx.currentTime - startTime;

      if (current < audioBuffer.duration) {
        setPosition(current);
        return;
      }

      // The track ran out: rewind so Play starts it again from the top.
      ctx.suspend();
      setIsPlaying(false);
      setOffset(0);
      setPosition(0);
    }, PROGRESS_INTERVAL_MS);

    return () => clearInterval(interval);
  }, [audioBuffer, isPlaying, offset, startTime]);

  const fetchAudioBuffer = async () => {
    const ctx = audioContextRef.current;
    if (!ctx) {
      return;
    }

    setIsLoading(true);

    const buffer = await fetch(URL)
      .then((response) => response.arrayBuffer())
      .then((arrayBuffer) => ctx.decodeAudioData(arrayBuffer))
      .catch((error) => {
        console.error('Error decoding audio data source:', error);
        return null;
      });

    setAudioBuffer(buffer);
    setIsLoading(false);
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    timeWorkletRef.current = new WorkletNode(
      audioContextRef.current!,
      (audioData) => {
        'worklet';

        const snapshot = timeDataSV.value;

        for (let i = 0; i < audioData.length; i++) {
          const sample = Math.max(-1, Math.min(1, audioData[i]!));
          snapshot[i] = Math.round((sample + 1) * 127.5);
        }

        timeDataTickSV.value += 1;
      },
      { domain: 'time-domain', bufferLength: FFT_SIZE }
    );

    frequencyWorkletRef.current = new WorkletNode(
      audioContextRef.current!,
      (audioData) => {
        'worklet';

        const snapshot = frequencyDataSV.value;

        for (let i = 0; i < audioData.length; i++) {
          snapshot[i] = linearMagnitudeToByte(audioData[i]!);
        }

        frequencyDataTickSV.value += 1;
      },
      {
        domain: 'frequency-domain',
        bufferLength: FREQUENCY_BIN_COUNT,
      }
    );
    frequencyWorkletRef.current.smoothingTimeConstant = 0.2;

    timeWorkletRef.current.connect(frequencyWorkletRef.current);
    frequencyWorkletRef.current.connect(audioContextRef.current!.destination);

    fetchAudioBuffer();
    setVisualizerReady(true);

    return () => {
      timeWorkletRef.current?.disconnect();
      frequencyWorkletRef.current?.disconnect();
      timeWorkletRef.current = null;
      frequencyWorkletRef.current = null;
      audioContextRef.current!.close();
      audioContextRef.current = null;
    };
  }, [frequencyDataSV, frequencyDataTickSV, timeDataSV, timeDataTickSV]);

  const duration = audioBuffer?.duration ?? 0;
  const progress = duration > 0 ? Math.min(1, position / duration) : 0;

  return (
    <Container disablePadding>
      <View style={[styles.chartArea, { paddingBottom: accessorySpace }]}>
        {visualizerReady ? (
          <FreqTimeChart
            timeDataSV={timeDataSV}
            timeDataTickSV={timeDataTickSV}
            frequencyDataSV={frequencyDataSV}
            frequencyDataTickSV={frequencyDataTickSV}
            fftSize={FFT_SIZE}
            frequencyBinCount={FREQUENCY_BIN_COUNT}
          />
        ) : null}
      </View>
      <BottomAccessory>
        <Pressable
          onPress={handlePlayPause}
          disabled={!audioBuffer}
          accessibilityRole="button"
          accessibilityLabel={isPlaying ? 'Pause' : 'Play'}
          accessibilityState={{ disabled: !audioBuffer }}
          style={({ pressed }) => [
            styles.playButton,
            pressed && styles.playButtonPressed,
          ]}
        >
          {isLoading ? (
            <ActivityIndicator color={colors.textPrimary} />
          ) : (
            <AppIcon
              name={isPlaying ? 'pause' : 'play'}
              size={22}
              color={audioBuffer ? colors.textPrimary : colors.textMuted}
            />
          )}
        </Pressable>
        <View style={styles.progressTrack}>
          <View
            style={[styles.progressFill, { width: `${progress * 100}%` }]}
          />
        </View>
        <Text style={styles.time}>
          {formatTime(position)} / {formatTime(duration)}
        </Text>
      </BottomAccessory>
    </Container>
  );
};

const styles = StyleSheet.create({
  chartArea: {
    flex: 1,
    width: '100%',
    justifyContent: 'center',
    alignItems: 'stretch',
  },
  playButton: {
    width: 40,
    height: 40,
    borderRadius: 20,
    alignItems: 'center',
    justifyContent: 'center',
  },
  playButtonPressed: {
    backgroundColor: colors.track,
  },
  progressTrack: {
    flex: 1,
    height: 4,
    borderRadius: 2,
    overflow: 'hidden',
    backgroundColor: colors.track,
  },
  progressFill: {
    height: '100%',
    borderRadius: 2,
    backgroundColor: colors.accent,
  },
  time: {
    marginRight: 10,
    fontSize: 13,
    fontVariant: ['tabular-nums'],
    color: colors.textSecondary,
  },
});

export default AudioVisualizer;
