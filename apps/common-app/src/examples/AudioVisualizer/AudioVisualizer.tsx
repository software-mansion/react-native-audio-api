import React, { useEffect, useRef, useState } from 'react';
import {
  ActivityIndicator,
  Pressable,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import { AudioBufferSourceNode } from 'react-native-audio-api';
import { WorkletNode } from 'react-native-audio-worklets';
import { useSharedValue } from 'react-native-reanimated';

import {
  AppIcon,
  Container,
  ControlsSheet,
  ProgressBar,
} from '../../components';
import { colors } from '../../styles';
import { useAudioContext, useDecodedAudio } from '../../utils/audio';
import { formatClock } from '../../utils/formatClock';
import FreqTimeChart from './FreqTimeChart';

const FFT_SIZE = 512;
const FREQUENCY_BIN_COUNT = FFT_SIZE / 2;

const URL =
  'https://docs.swmansion.com/react-native-audio-api/audio/music/example-music-02.mp3';

/** How often the elapsed time and progress line refresh while playing. */
const PROGRESS_INTERVAL_MS = 250;

const ANALYSER_MIN_DB = -100;
const ANALYSER_MAX_DB = -30;

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

  const [startTime, setStartTime] = useState(0);
  const [offset, setOffset] = useState(0);
  /** Seconds into the track, refreshed while playing. */
  const [position, setPosition] = useState(0);
  const [sheetHeight, setSheetHeight] = useState(0);

  const audioContext = useAudioContext();
  const audioBuffer = useDecodedAudio(audioContext, URL);
  const timeWorkletRef = useRef<WorkletNode | null>(null);
  const frequencyWorkletRef = useRef<WorkletNode | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);

  const timeDataSV = useSharedValue(new Uint8Array(FFT_SIZE).fill(127));
  const frequencyDataSV = useSharedValue(
    new Uint8Array(FREQUENCY_BIN_COUNT).fill(0)
  );

  const handlePlayPause = async () => {
    if (isPlaying) {
      const stopTime = audioContext.currentTime;
      audioContext.suspend();
      bufferSourceRef.current?.stop(stopTime);
      bufferSourceRef.current?.disconnect();
      const pausedAt = offset + stopTime - startTime;
      setOffset(pausedAt);
      setPosition(pausedAt);
      setIsPlaying(false);
      return;
    }

    const timeWorklet = timeWorkletRef.current;

    if (!timeWorklet || !audioBuffer) {
      return;
    }

    await audioContext.resume();

    bufferSourceRef.current = audioContext.createBufferSource();
    bufferSourceRef.current.buffer = audioBuffer;
    bufferSourceRef.current.connect(timeWorklet);

    const when = audioContext.currentTime;
    setStartTime(when);
    bufferSourceRef.current.start(when, offset);
    setIsPlaying(true);
  };

  useEffect(() => {
    if (!isPlaying || !audioBuffer) {
      return;
    }

    const interval = setInterval(() => {
      const current = offset + audioContext.currentTime - startTime;

      if (current < audioBuffer.duration) {
        setPosition(current);
        return;
      }

      // The track ran out: rewind so Play starts it again from the top.
      audioContext.suspend();
      bufferSourceRef.current?.disconnect();
      setIsPlaying(false);
      setOffset(0);
      setPosition(0);
    }, PROGRESS_INTERVAL_MS);

    return () => clearInterval(interval);
  }, [audioBuffer, audioContext, isPlaying, offset, startTime]);

  useEffect(() => {
    timeWorkletRef.current = new WorkletNode(
      audioContext,
      (audioData) => {
        'worklet';

        timeDataSV.modify((snapshot) => {
          'worklet';
          for (let i = 0; i < audioData.length; i++) {
            const sample = Math.max(-1, Math.min(1, audioData[i]!));
            snapshot[i] = Math.round((sample + 1) * 127.5);
          }
          return snapshot;
        });
      },
      { domain: 'time-domain', bufferLength: FFT_SIZE }
    );

    frequencyWorkletRef.current = new WorkletNode(
      audioContext,
      (audioData) => {
        'worklet';

        frequencyDataSV.modify((snapshot) => {
          'worklet';
          for (let i = 0; i < audioData.length; i++) {
            snapshot[i] = linearMagnitudeToByte(audioData[i]!);
          }
          return snapshot;
        });
      },
      {
        domain: 'frequency-domain',
        bufferLength: FREQUENCY_BIN_COUNT,
      }
    );
    frequencyWorkletRef.current.smoothingTimeConstant = 0.2;

    timeWorkletRef.current.connect(frequencyWorkletRef.current);
    frequencyWorkletRef.current.connect(audioContext.destination);

    return () => {
      bufferSourceRef.current?.disconnect();
      timeWorkletRef.current?.disconnect();
      frequencyWorkletRef.current?.disconnect();
      timeWorkletRef.current = null;
      frequencyWorkletRef.current = null;
    };
  }, [audioContext, frequencyDataSV, timeDataSV]);

  const duration = audioBuffer?.duration ?? 0;
  const progress = duration > 0 ? position / duration : 0;

  return (
    <Container disablePadding>
      <View style={[styles.chartArea, { paddingBottom: sheetHeight }]}>
        <FreqTimeChart
          timeDataSV={timeDataSV}
          frequencyDataSV={frequencyDataSV}
          fftSize={FFT_SIZE}
          frequencyBinCount={FREQUENCY_BIN_COUNT}
        />
      </View>
      <ControlsSheet onCoveredHeightChange={setSheetHeight}>
        <View style={styles.transport}>
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
            {!audioBuffer ? (
              <ActivityIndicator color={colors.textPrimary} />
            ) : (
              <AppIcon
                name={isPlaying ? 'pause' : 'play'}
                size={22}
                color={colors.textPrimary}
              />
            )}
          </Pressable>
          <ProgressBar progress={progress} style={styles.progress} />
          <Text style={styles.time}>
            {formatClock(position)} / {formatClock(duration)}
          </Text>
        </View>
      </ControlsSheet>
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
  transport: {
    height: 56,
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 8,
    gap: 12,
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
  progress: {
    flex: 1,
  },
  time: {
    marginRight: 10,
    fontSize: 13,
    fontVariant: ['tabular-nums'],
    color: colors.textSecondary,
  },
});

export default AudioVisualizer;
