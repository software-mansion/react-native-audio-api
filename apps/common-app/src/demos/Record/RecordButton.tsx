import React, { FC, useCallback } from 'react';
import { Pressable, StyleSheet } from 'react-native';
import Animated, {
  useAnimatedProps,
  withRepeat,
  withSequence,
  withSpring,
  withTiming,
} from 'react-native-reanimated';
import Svg, { Path } from 'react-native-svg';

import { RecordingState } from './types';

interface RecordButtonProps {
  state: RecordingState;
  onToggleState: (action: RecordingState) => void;
}

const RecordButton: FC<RecordButtonProps> = ({ state, onToggleState }) => {
  const onPress = useCallback(() => {
    if ([RecordingState.Idle, RecordingState.Paused].includes(state)) {
      onToggleState(RecordingState.Recording);
      return;
    }

    if (state === RecordingState.Recording) {
      onToggleState(RecordingState.ReadyToPlay);
      return;
    }

    if (state === RecordingState.ReadyToPlay) {
      onToggleState(RecordingState.Playing);
      return;
    }

    if (state === RecordingState.Playing) {
      onToggleState(RecordingState.Idle);
    }
  }, [onToggleState, state]);

  return (
    <Pressable onPress={onPress}>
      {({ pressed }) => <RecordButtonInner pressed={pressed} state={state} />}
    </Pressable>
  );
};

export default RecordButton;

type OuterStyle = {
  borderColor: string;
  transform: {
    scale: number;
  }[];
};

const RecordButtonInner: FC<{
  pressed: boolean;
  state: RecordingState;
}> = ({ pressed, state }) => {
  const outerViewStyle = useAnimatedProps((): OuterStyle => {
    if (state === RecordingState.Loading) {
      return {
        borderColor: '#d4d4d4',
        transform: [
          {
            scale: withRepeat(
              withSequence(withSpring(1), withSpring(0.3)),
              -1,
              true
            ),
          },
        ],
      };
    }

    return {
      borderColor: withTiming(
        state === RecordingState.ReadyToPlay ? '#ff6259' : 'white'
      ),
      transform: [
        {
          scale: pressed ? withSpring(0.9) : withSpring(1),
        },
      ],
    };
  });

  const innerViewStyle = useAnimatedProps(() => {
    let size = 40;

    if (
      state === RecordingState.Recording ||
      state === RecordingState.Playing
    ) {
      size = 32;
    }

    if (state === RecordingState.ReadyToPlay) {
      size = 0;
    }

    const transform = (() => {
      if (state === RecordingState.Loading) {
        return [
          {
            scale: withRepeat(
              withSequence(withSpring(1), withSpring(5)),
              -1,
              true
            ),
          },
        ];
      }

      if (pressed) {
        return [
          {
            scale: withSpring(1.6),
          },
        ];
      }

      return [
        {
          scale: withSpring(1),
        },
      ];
    })();

    return {
      borderRadius:
        state === RecordingState.Recording || state === RecordingState.Playing
          ? withTiming(4)
          : withTiming(20),
      width: withTiming(size),
      height: withTiming(size),
      transform,
    };
  });

  const playStyle = useAnimatedProps(() => {
    const scale = state === RecordingState.ReadyToPlay ? 1 : 0;
    return {
      transform: [
        {
          scale: withTiming(scale),
        },
      ],
    };
  });

  return (
    <Animated.View style={[styles.innerContainer, outerViewStyle]}>
      <Animated.View style={[styles.mainIcon, innerViewStyle]} />
      <Animated.View style={[styles.playIconContainer, playStyle]}>
        <Svg width={72} height={72} viewBox="0 0 24 24">
          <Path
            d="M10 8 L16 12 L10 16 Z"
            fill="#d4d4d4"
            stroke="#d4d4d4"
            strokeWidth={4}
            strokeLinejoin="round" // <-- rounded corners
          />
        </Svg>
      </Animated.View>
    </Animated.View>
  );
};

const styles = StyleSheet.create({
  innerContainer: {
    width: 60,
    height: 60,
    borderRadius: 30,
    borderWidth: 2,
    alignItems: 'center',
    justifyContent: 'center',
  },
  mainIcon: {
    position: 'absolute',
    width: 40,
    height: 40,
    borderRadius: 20,
    backgroundColor: '#ff6259',
  },
  playIconContainer: {
    position: 'absolute',
    top: -8,
    left: -8,
    overflow: 'hidden',
  },
});
