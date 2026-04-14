import React from 'react';
import { View, ViewStyle, Image } from 'react-native';
import Animated, {
  useAnimatedStyle,
  withTiming,
} from 'react-native-reanimated';

type FlapParams = {
  axis: 'X' | 'Y';
  translate: number;
  multiplier: number;
};

const useFlapStyle = (
  progress: number,
  { axis, translate, multiplier }: FlapParams
) => {
  return useAnimatedStyle(() => {
    const rotate = `${progress * multiplier}deg`;
    return {
      transform: [
        { perspective: 800 },
        axis === 'X' ? { translateY: translate } : { translateX: translate },
        axis === 'X'
          ? { rotateX: withTiming(rotate, { duration: 50 }) }
          : { rotateY: withTiming(rotate, { duration: 50 }) },
        axis === 'X' ? { translateY: -translate } : { translateX: -translate },
      ],
    };
  });
};

export const BoxWithFlaps = ({ progress }: { progress: number }) => {
  const phaseShift = 0.7;
  const delayedProgress = Math.max(0, progress - phaseShift) / (1 - phaseShift);

  const topStyle = useFlapStyle(delayedProgress, { axis: 'X', translate: -25, multiplier: 130 });
  const bottomStyle = useFlapStyle(delayedProgress, { axis: 'X', translate: 25, multiplier: -130 });
  const leftStyle = useFlapStyle(progress, { axis: 'Y', translate: -50, multiplier: -110 });
  const rightStyle = useFlapStyle(progress, { axis: 'Y', translate: 50, multiplier: 110 });

  const baseFlapStyle: ViewStyle = {
    position: 'absolute',
    backgroundColor: '#a9744f',
    borderColor: '#5c3d2e',
  };

  const flapConfigs = [
    {
      key: 'top',
      style: { top: 0, left: 1, width: 198, height: 50, borderBottomWidth: 2 },
      animatedStyle: topStyle,
    },
    {
      key: 'bottom',
      style: { bottom: 0, left: 1, width: 198, height: 50, borderTopWidth: 2 },
      animatedStyle: bottomStyle,
    },
    {
      key: 'left',
      style: { top: 0, left: 0, width: 100, height: 200, borderRightWidth: 2, zIndex: 1 },
      animatedStyle: leftStyle,
    },
    {
      key: 'right',
      style: { top: 0, right: 0, width: 100, height: 200, borderLeftWidth: 2, zIndex: 1 },
      animatedStyle: rightStyle,
    },
  ];

  return (
    <View
      style={{
        width: 200,
        height: 200,
        position: 'relative',
        alignItems: 'center',
        justifyContent: 'center',
      }}>
      {/* Box base */}
      <View
        style={{
          position: 'absolute',
          width: 200,
          height: 200,
          backgroundColor: '#9d6c49ff',
          borderWidth: 2,
          borderColor: '#5c3d2e',
          borderRadius: 4,
        }}
      />

      {/* Gramophone */}
      <Image
        style={{ width: 250, height: 250, position: 'absolute' }}
        alt="gramophone"
        source={{
          uri: '/react-native-audio-api/img/gramophone.png',
        }}
      />

      {/* Flaps */}
      {flapConfigs.map((flap) => (
        <Animated.View
          key={flap.key}
          style={[baseFlapStyle, flap.style, flap.animatedStyle]}
        />
      ))}
    </View>
  );
};
