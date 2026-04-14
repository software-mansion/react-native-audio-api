import React, { useEffect } from 'react';
import { StyleSheet, TextInput } from 'react-native';
import Animated, {
  useAnimatedProps,
  useSharedValue,
} from 'react-native-reanimated';

import { audioRecorder as Recorder } from '../../singletons';
import { colors } from '../../styles';
import { RecordingState } from './types';

const AnimatedTextInput = Animated.createAnimatedComponent(TextInput);

interface RecordingTimeProps {
  state: RecordingState;
}

const RecordingTime: React.FC<RecordingTimeProps> = ({ state }) => {
  const durationStringSV = useSharedValue('00:00:000');
  const isMountedSV = useSharedValue(true);

  useEffect(() => {
    isMountedSV.value = true;
    if (![RecordingState.Recording, RecordingState.Paused].includes(state)) {
      durationStringSV.value = '00:00:00';
      return;
    }

    const interval = setInterval(() => {
      if (!isMountedSV.value) {
        return;
      }

      const elapsedSeconds = Recorder.getCurrentDuration();

      const minutes = Math.floor((elapsedSeconds % 3600) / 60)
        .toString()
        .padStart(2, '0');
      const seconds = Math.floor(elapsedSeconds % 60)
        .toString()
        .padStart(2, '0');
      const milliseconds = Math.floor((elapsedSeconds % 1) * 1000)
        .toString()
        .padStart(3, '0');

      durationStringSV.value = `${minutes}:${seconds}:${milliseconds}`;
    }, 100);

    return () => {
      isMountedSV.value = false;
      clearInterval(interval);
    };
  }, [state, durationStringSV, isMountedSV]);

  const animatedText = useAnimatedProps(() => {
    return {
      text: durationStringSV.value,
      defaultValue: '00:00:000',
    };
  });

  return (
    <AnimatedTextInput
      editable={false}
      animatedProps={animatedText}
      style={styles.text}
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
