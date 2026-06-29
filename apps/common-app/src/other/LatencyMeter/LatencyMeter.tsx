import React, { FC, useCallback, useEffect, useRef, useState } from 'react';
import { Platform, StyleSheet, Text, View } from 'react-native';
import { AudioManager } from 'react-native-audio-api';

import { Button, Container } from '../../components';
import { audioContext, audioRecorder } from '../../singletons';
import { colors } from '../../styles';

interface Latencies {
  base: number;
  output: number;
  input: number;
}

const fmt = (s: number) => `${(s * 1000).toFixed(2)} ms`;

const LatencyMeter: FC = () => {
  const [isRunning, setIsRunning] = useState(false);
  const [latencies, setLatencies] = useState<Latencies | null>(null);
  const intervalRef = useRef<ReturnType<typeof setInterval> | null>(null);

  const readLatencies = useCallback(() => {
    setLatencies({
      base: audioContext.baseLatency,
      output: audioContext.outputLatency,
      input: audioRecorder.inputLatency,
    });
  }, []);

  const start = useCallback(async () => {
    const status = await AudioManager.requestRecordingPermissions();
    if (status !== 'Granted') {
      return;
    }

    if (Platform.OS !== 'web') {
      AudioManager.setAudioSessionOptions({
        iosCategory: 'playAndRecord',
        iosMode: 'measurement',
        iosOptions: ['allowBluetooth'],
      });
      await AudioManager.setAudioSessionActivity(true);
    }

    if (audioContext.state === 'suspended') {
      await audioContext.resume();
    }

    await audioRecorder.start();
    setIsRunning(true);
    readLatencies();
    intervalRef.current = setInterval(readLatencies, 500);
  }, [readLatencies]);

  const stop = useCallback(async () => {
    if (intervalRef.current) {
      clearInterval(intervalRef.current);
      intervalRef.current = null;
    }
    await audioRecorder.stop();
    if (Platform.OS !== 'web') {
      await AudioManager.setAudioSessionActivity(false);
    }
    setIsRunning(false);
  }, []);

  useEffect(() => {
    return () => {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
      }
    };
  }, []);

  return (
    <Container centered>
      <Button
        title={isRunning ? 'Stop' : 'Start'}
        onPress={isRunning ? stop : start}
      />
      {latencies && (
        <View style={styles.table}>
          {(
            [
              ['Base latency', latencies.base],
              ['Output latency', latencies.output],
              ['Input latency', latencies.input],
            ] as [string, number][]
          ).map(([label, value]) => (
            <View key={label} style={styles.row}>
              <Text style={styles.label}>{label}</Text>
              <Text style={styles.value}>{fmt(value)}</Text>
            </View>
          ))}
        </View>
      )}
    </Container>
  );
};

const styles = StyleSheet.create({
  table: {
    marginTop: 32,
    width: '100%',
    paddingHorizontal: 24,
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 14,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: colors.border,
  },
  label: {
    fontSize: 16,
    color: colors.white,
  },
  value: {
    fontSize: 16,
    fontVariant: ['tabular-nums'],
    color: colors.main,
  },
});

export default LatencyMeter;
