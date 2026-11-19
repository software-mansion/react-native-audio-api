import React, { useEffect, useMemo } from 'react';
import { StyleSheet, TextInput } from 'react-native';
import Animated, {
  useAnimatedProps,
  useFrameCallback,
  useSharedValue,
} from 'react-native-reanimated';

import { audioRecorder as Recorder } from '../../singletons';
import { colors } from '../../styles';
import { RecordingState } from './types';

const ZERO_DURATION = '00:00:000';

const AnimatedTextInput = Animated.createAnimatedComponent(TextInput);

interface RecordingTimeProps {
  state: RecordingState;
}

function formatDuration(elapsedSeconds: number): string {
  'worklet';
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
  const recordingClock = useMemo(() => Recorder.clock, []);
  const duration = useSharedValue(ZERO_DURATION);
  const isTicking =
    state === RecordingState.Recording || state === RecordingState.Paused;

  const ticker = useFrameCallback(() => {
    'worklet';
    const next = formatDuration(recordingClock.currentDuration);

    if (next !== duration.value) {
      duration.value = next;
    }
  }, false);

  useEffect(() => {
    ticker.setActive(isTicking);
    if (!isTicking) {
      duration.value = ZERO_DURATION;
    }
  }, [isTicking, ticker, duration]);

  const animatedProps = useAnimatedProps(() => ({
    text: duration.value,
    defaultValue: duration.value,
  }));

  return (
    <AnimatedTextInput
      style={styles.text}
      editable={false}
      animatedProps={animatedProps}
    />
  );
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
