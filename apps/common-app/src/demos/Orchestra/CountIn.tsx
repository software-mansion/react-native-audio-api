import React, { FC, useEffect, useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { colors } from '../../styles';
import type ClockSync from './ClockSync';
import type { Transport } from './protocol';

interface CountInProps {
  transport: Transport | undefined;
  clockSync: ClockSync;
  accent: string;
}

const TICK_MS = 50;

/** Counts down the seconds to a start every phone heard about, over the roll. */
const CountIn: FC<CountInProps> = ({ transport, clockSync, accent }) => {
  const [secondsLeft, setSecondsLeft] = useState<number | null>(null);

  const playing = transport?.playing ?? false;
  const startAt = transport?.startAt ?? 0;
  useEffect(() => {
    if (!playing) {
      setSecondsLeft(null);
      return;
    }
    const tick = () => {
      const remainingMs = clockSync.serverToLocal(startAt) - performance.now();
      setSecondsLeft(remainingMs > 0 ? Math.ceil(remainingMs / 1000) : null);
      return remainingMs > 0;
    };
    if (!tick()) {
      return;
    }
    const timer = setInterval(() => {
      if (!tick()) {
        clearInterval(timer);
      }
    }, TICK_MS);
    return () => clearInterval(timer);
  }, [playing, startAt, clockSync]);

  if (secondsLeft === null) {
    return null;
  }
  return (
    <View style={styles.overlay} pointerEvents="none">
      <Text style={[styles.number, { color: accent }]}>{secondsLeft}</Text>
    </View>
  );
};

export default CountIn;

const styles = StyleSheet.create({
  overlay: {
    ...StyleSheet.absoluteFill,
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: `${colors.background}cc`,
  },
  number: {
    fontSize: 120,
    fontWeight: '800',
    fontVariant: ['tabular-nums'],
  },
});
