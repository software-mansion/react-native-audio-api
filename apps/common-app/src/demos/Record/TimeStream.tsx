import React, { useEffect, useRef, useState } from 'react';
import { Dimensions, StyleSheet, Text, View } from 'react-native';
import Animated, {
  cancelAnimation,
  Easing,
  SharedValue,
  useAnimatedStyle,
  useSharedValue,
  withTiming,
} from 'react-native-reanimated';
import constants from './constants';

const windowWidth = Dimensions.get('window').width;

const formatTime = (seconds: number) => {
  const m = Math.floor(seconds / 60);
  const s = seconds % 60;
  return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
};

interface TimeStreamProps {
  isRecording: boolean;
  durationMS: SharedValue<number>;
}

function generateInitialTimestamps() {
  const timestamps: number[] = [];

  for (let i = -5; i < 15; i++) {
    timestamps.push(i);
  }

  return timestamps;
}

const TimeStream: React.FC<TimeStreamProps> = ({ isRecording, durationMS }) => {
  const [timestamps, setTimestamps] = useState<number[]>(
    generateInitialTimestamps()
  );
  const intervalRef = useRef<NodeJS.Timeout | null>(null);

  useEffect(() => {
    if (isRecording) {
      setTimestamps(generateInitialTimestamps());

      intervalRef.current = setInterval(() => {
        const elapsedSeconds = durationMS.value / 1000;
        const futureSecond = Math.ceil(elapsedSeconds + 1);

        setTimestamps((prev) => {
          if (prev.includes(futureSecond)) {
            return prev;
          }

          const cleanList = prev.filter((t) => t > elapsedSeconds - 5);
          return [...cleanList, futureSecond];
        });
      }, 500);
    } else {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
      }
    }

    return () => {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
      }
    };
  }, [isRecording, durationMS]);

  return (
    <View style={StyleSheet.absoluteFill} pointerEvents="none">
      {timestamps.map((seconds) => (
        <Timestamp
          key={seconds}
          spawnSeconds={seconds}
          durationMS={durationMS}
          isRecording={isRecording}
        />
      ))}
    </View>
  );
};

export default TimeStream;

const textWidth = 60;

interface TimestampProps {
  spawnSeconds: number;
  durationMS: SharedValue<number>;
  isRecording: boolean;
}

const subSeconds = new Array(7).fill(0).map((_, i) => `sub-${i}`);

const Timestamp: React.FC<TimestampProps> = ({
  spawnSeconds,
  durationMS,
  isRecording,
}) => {
  const translateX = useSharedValue(2 * windowWidth);

  useEffect(() => {
    const originalPositionOfFirstTimestamp = windowWidth - textWidth / 2;
    const currentPositionOfFirstTimestamp =
      originalPositionOfFirstTimestamp -
      durationMS.value * constants.pixelsPerMS;

    const startX =
      currentPositionOfFirstTimestamp +
      spawnSeconds * constants.pixelsPerSecond;

    const endX = -constants.pixelsPerSecond * 2;

    const totalDistance = startX - endX;
    const duration = (totalDistance / constants.pixelsPerSecond) * 1000;

    translateX.value = startX;

    if (!isRecording) {
      cancelAnimation(translateX);
      return;
    }

    translateX.value = withTiming(endX, {
      duration: duration,
      easing: Easing.linear,
    });
  }, [spawnSeconds, durationMS, translateX, isRecording]);

  const containerStyle = useAnimatedStyle(() => ({
    position: 'absolute',
    bottom: 5,
    left: 0,
    width: textWidth,
    alignItems: 'center',
    transform: [{ translateX: translateX.value }],
  }));

  return (
    <>
      <Animated.View style={containerStyle}>
        <View style={styles.smallIndicatorContainer}>
          {subSeconds.map((key, index) => (
            <View
              key={key}
              style={[
                styles.subSecondIndicator,
                (index === 0 || index === subSeconds.length - 1) &&
                  styles.hidden,
              ]}
            />
          ))}
        </View>
        <View style={styles.timeIndicatorLarge} />
        <Text style={[styles.text, spawnSeconds < 0 && styles.hidden]}>
          {formatTime(spawnSeconds)}
        </Text>
      </Animated.View>
    </>
  );
};

const styles = StyleSheet.create({
  text: {
    color: '#888',
    fontSize: 16,
    fontFamily: 'Menlo',
    fontWeight: '600',
    textAlign: 'center',
  },
  timeIndicatorLarge: {
    width: 2,
    height: 20,
    backgroundColor: 'rgba(255,255,255,0.3)',
    marginBottom: 4,
  },
  smallIndicatorContainer: {
    position: 'absolute',
    flexDirection: 'row',
    justifyContent: 'space-between',
    width: constants.pixelsPerSecond + 2,
    height: 15,
    top: 10,
    left: textWidth / 2 - 1,
  },
  subSecondIndicator: {
    height: 10,
    width: 2,
    backgroundColor: 'rgba(255,255,255, 0.3)',
    marginBottom: 4,
  },
  hidden: {
    opacity: 0,
  },
});
