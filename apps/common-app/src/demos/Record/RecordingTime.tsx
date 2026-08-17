import React, { useEffect, useState } from 'react';
import { StyleSheet, Text } from 'react-native';

import { audioRecorder as Recorder } from '../../singletons';
import { colors } from '../../styles';
import { RecordingState } from './types';

const IDLE_DURATION = '00:00:000';

function formatDuration(elapsedSeconds: number) {
  const minutes = Math.floor((elapsedSeconds % 3600) / 60)
    .toString()
    .padStart(2, '0');
  const seconds = Math.floor(elapsedSeconds % 60)
    .toString()
    .padStart(2, '0');
  const milliseconds = Math.floor((elapsedSeconds % 1) * 1000)
    .toString()
    .padStart(3, '0');

  return `${minutes}:${seconds}:${milliseconds}`;
}

interface RecordingTimeProps {
  state: RecordingState;
}

const RecordingTime: React.FC<RecordingTimeProps> = ({ state }) => {
  const [durationString, setDurationString] = useState(IDLE_DURATION);

  useEffect(() => {
    if (![RecordingState.Recording, RecordingState.Paused].includes(state)) {
      setDurationString(IDLE_DURATION);
      return;
    }

    const refreshDuration = () =>
      setDurationString(formatDuration(Recorder.getCurrentDuration()));

    // Also refresh immediately so a paused or resynced screen shows the real
    // duration before the first interval tick.
    refreshDuration();
    const interval = setInterval(refreshDuration, 100);

    return () => {
      clearInterval(interval);
    };
  }, [state]);

  return <Text style={styles.text}>{durationString}</Text>;
};

export default RecordingTime;

const styles = StyleSheet.create({
  text: {
    color: colors.gray,
    fontSize: 48,
    width: '100%',
    textAlign: 'center',
    fontFamily: 'courier-new',
    fontWeight: 'bold',
    fontVariant: ['tabular-nums'],
  },
});
