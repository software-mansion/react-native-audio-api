import React, { useEffect, useState } from 'react';
import { StyleSheet, Text } from 'react-native';

import { audioRecorder as Recorder } from '../../singletons';
import { colors } from '../../styles';
import { RecordingState } from './types';

const ZERO_DURATION = '00:00:000';

interface RecordingTimeProps {
  state: RecordingState;
}

function formatDuration(elapsedSeconds: number): string {
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

const RecordingTime: React.FC<RecordingTimeProps> = ({ state }) => {
  const [duration, setDuration] = useState(ZERO_DURATION);

  useEffect(() => {
    if (![RecordingState.Recording, RecordingState.Paused].includes(state)) {
      setDuration(ZERO_DURATION);
      return;
    }

    const showCurrentDuration = () =>
      setDuration(formatDuration(Recorder.getCurrentDuration()));

    showCurrentDuration();
    const interval = setInterval(showCurrentDuration, 100);
    return () => clearInterval(interval);
  }, [state]);

  return <Text style={styles.text}>{duration}</Text>;
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
