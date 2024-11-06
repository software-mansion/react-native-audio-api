import Animated, {
  withTiming,
  SharedValue,
  useAnimatedStyle,
} from 'react-native-reanimated';
import React, { memo } from 'react';
// @ts-expect-error
import { Icon } from '@swmansion/icons';
import { Pressable, StyleSheet } from 'react-native';

import { colors } from '../../styles';
import type { PlayingInstruments, XYWHRect } from './types';
import { size } from './constants';

interface PlayButtonProps {
  isPlaying?: boolean;
  onPress: () => void;

  canvasRect: XYWHRect;
  playingInstruments: SharedValue<PlayingInstruments>;
}

interface PlayButtonInnerProps {
  pressed: boolean;
  isPlaying?: boolean;
  playingInstruments: SharedValue<PlayingInstruments>;
}

const timingOptions = {
  duration: 25,
};

const PlayButtonInner: React.FC<PlayButtonInnerProps> = (props) => {
  const { pressed, isPlaying, playingInstruments } = props;

  const containerStyle = useAnimatedStyle(() => {
    const shouldPlay = playingInstruments.value.kick;

    return {
      transform: [
        {
          scale: withTiming(pressed || shouldPlay ? 1.15 : 1, timingOptions),
        },
      ],
    };
  });

  return (
    <Animated.View style={[styles.playButtonInner, containerStyle]}>
      <Icon
        size={48}
        type="broken"
        color={colors.white}
        name={isPlaying ? 'pause' : 'play'}
      />
    </Animated.View>
  );
};

const PlayButton: React.FC<PlayButtonProps> = (props) => {
  const { canvasRect, onPress, isPlaying, playingInstruments } = props;

  return (
    <Pressable
      style={[
        styles.playButton,
        {
          top: canvasRect.y + size / 2 - 30,
          left: canvasRect.x + size / 2 - 30,
        },
      ]}
      onPress={onPress}
    >
      {({ pressed }) => (
        <PlayButtonInner
          pressed={pressed}
          isPlaying={isPlaying}
          playingInstruments={playingInstruments}
        />
      )}
    </Pressable>
  );
};

export default memo(PlayButton);

const styles = StyleSheet.create({
  playButton: {
    width: 60,
    height: 60,
    borderRadius: 30,
    position: 'absolute',
  },
  playButtonInner: {
    backgroundColor: colors.main,
    width: 60,
    height: 60,
    borderRadius: 30,
    justifyContent: 'center',
    alignItems: 'center',
    borderWidth: StyleSheet.hairlineWidth,
    borderColor: colors.white,
  },
  playButtonText: {
    color: colors.white,
    fontSize: 18,
  },
});
