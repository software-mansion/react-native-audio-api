import React, { memo, useState } from 'react';
import {
  AccessibilityActionEvent,
  LayoutChangeEvent,
  StyleSheet,
  View,
} from 'react-native';
import { GestureDetector, usePanGesture } from 'react-native-gesture-handler';
import Animated, {
  AnimatedRef,
  scrollTo,
  SharedValue,
  useAnimatedStyle,
} from 'react-native-reanimated';

import { colors } from '../../styles';
import { blackKeys, blackKeyWidth, keyboardWidth, whiteKeys } from './keys';

const height = 40;

const accessibilityActions = [
  { name: 'increment' as const },
  { name: 'decrement' as const },
];

interface MiniKeysProps {
  width: number;
  pressedKeys: ReadonlySet<number>;
}

const MiniKeys = memo(function MiniKeys({ width, pressedKeys }: MiniKeysProps) {
  const scale = width / keyboardWidth;

  return (
    <>
      <View style={styles.whiteRow}>
        {whiteKeys.map((key) => (
          <View
            key={key.midi}
            style={[
              styles.miniWhite,
              pressedKeys.has(key.midi) && styles.miniPressed,
            ]}
          />
        ))}
      </View>
      {blackKeys.map((key) => (
        <View
          key={key.midi}
          style={[
            styles.miniBlack,
            { left: key.x * scale, width: blackKeyWidth * scale },
            pressedKeys.has(key.midi) && styles.miniPressed,
          ]}
        />
      ))}
    </>
  );
});

interface KeyboardOverviewProps {
  scrollRef: AnimatedRef<Animated.ScrollView>;
  scrollOffset: SharedValue<number>;
  viewportWidth: number;
  pressedKeys: ReadonlySet<number>;
  octaveLabel: string;
  onStepOctave: (direction: 1 | -1) => void;
}

/**
 * A miniature of the whole keyboard with a window marking the visible part.
 * Touching or dragging it centres the keyboard on that point; this is the
 * scroll gesture, because dragging on the keys themselves plays them.
 */
const KeyboardOverview: React.FC<KeyboardOverviewProps> = ({
  scrollRef,
  scrollOffset,
  viewportWidth,
  pressedKeys,
  octaveLabel,
  onStepOctave,
}) => {
  const [width, setWidth] = useState(0);
  const scale = width / keyboardWidth;
  const maxOffset = Math.max(0, keyboardWidth - viewportWidth);

  const scrollToPoint = (x: number) => {
    'worklet';
    if (scale === 0) {
      return;
    }

    const target = x / scale - viewportWidth / 2;
    scrollTo(scrollRef, Math.min(maxOffset, Math.max(0, target)), 0, false);
  };

  const pan = usePanGesture({
    minDistance: 0,
    onBegin: (event) => scrollToPoint(event.x),
    onUpdate: (event) => scrollToPoint(event.x),
  });

  const windowStyle = useAnimatedStyle(() => ({
    transform: [{ translateX: scrollOffset.value * scale }],
  }));

  const onLayout = (event: LayoutChangeEvent) => {
    setWidth(event.nativeEvent.layout.width);
  };

  const onAccessibilityAction = (event: AccessibilityActionEvent) => {
    onStepOctave(event.nativeEvent.actionName === 'increment' ? 1 : -1);
  };

  return (
    <GestureDetector gesture={pan}>
      <View
        style={styles.strip}
        onLayout={onLayout}
        accessible
        accessibilityRole="adjustable"
        accessibilityLabel="Keyboard position"
        accessibilityValue={{ text: octaveLabel }}
        accessibilityActions={accessibilityActions}
        onAccessibilityAction={onAccessibilityAction}
      >
        {width > 0 && <MiniKeys width={width} pressedKeys={pressedKeys} />}
        <Animated.View
          pointerEvents="none"
          style={[
            styles.window,
            { width: Math.min(width, viewportWidth * scale) },
            windowStyle,
          ]}
        />
      </View>
    </GestureDetector>
  );
};

export default KeyboardOverview;

const styles = StyleSheet.create({
  strip: {
    height,
    borderRadius: 10,
    overflow: 'hidden',
    backgroundColor: '#0f1016',
  },
  whiteRow: {
    ...StyleSheet.absoluteFill,
    flexDirection: 'row',
  },
  miniWhite: {
    flex: 1,
    marginRight: StyleSheet.hairlineWidth,
    backgroundColor: '#d8d7d1',
  },
  miniBlack: {
    position: 'absolute',
    top: 0,
    height: height * 0.6,
    backgroundColor: '#0f1016',
  },
  miniPressed: {
    backgroundColor: colors.accent,
  },
  window: {
    position: 'absolute',
    top: 0,
    bottom: 0,
    borderRadius: 10,
    borderWidth: 2,
    borderColor: colors.accent,
    backgroundColor: 'rgba(255, 119, 116, 0.12)',
  },
});
