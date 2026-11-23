import React, { useEffect, useState } from 'react';
import { StyleSheet, Text } from 'react-native';

import { audioRecorder as Recorder } from '../../singletons';
import { colors } from '../../styles';
import { formatClock } from '../../utils/formatClock';
import { RecordingState } from './types';

const refreshIntervalMs = 50;

interface RecordingTimerProps {
  state: RecordingState;
}

/** Elapsed recording time as `mm:ss` with dimmed centiseconds. */
const RecordingTimer: React.FC<RecordingTimerProps> = ({ state }) => {
  const [elapsed, setElapsed] = useState(0);
  const isActive =
    state === RecordingState.Recording || state === RecordingState.Paused;

  useEffect(() => {
    if (!isActive) {
      setElapsed(0);
      return;
    }

    setElapsed(Recorder.getCurrentDuration());
    const interval = setInterval(
      () => setElapsed(Recorder.getCurrentDuration()),
      refreshIntervalMs
    );
    return () => clearInterval(interval);
  }, [isActive]);

  const centiseconds = String(Math.floor((elapsed % 1) * 100)).padStart(2, '0');

  return (
    <Text
      style={styles.timer}
      accessibilityLabel={`${Math.floor(elapsed)} seconds recorded`}
    >
      {formatClock(elapsed, true)}
      <Text style={styles.centiseconds}>.{centiseconds}</Text>
    </Text>
  );
};

export default RecordingTimer;

const styles = StyleSheet.create({
  timer: {
    fontSize: 64,
    fontWeight: '200',
    letterSpacing: -1.5,
    fontVariant: ['tabular-nums'],
    textAlign: 'center',
    color: colors.textPrimary,
  },
  centiseconds: {
    color: colors.textMuted,
  },
});
