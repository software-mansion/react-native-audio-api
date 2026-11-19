import React, { memo } from 'react';
import { LayoutChangeEvent, StyleSheet, Text, View } from 'react-native';
import {
  GestureDetector,
  GestureStateManager,
  GestureTouchEvent,
  useManualGesture,
} from 'react-native-gesture-handler';
import Animated, { AnimatedRef, useSharedValue } from 'react-native-reanimated';
import { scheduleOnRN } from 'react-native-worklets';

import {
  blackKeyHeight,
  blackKeys,
  blackKeyWidth,
  keyAt,
  keyboardWidth,
  PianoKey,
  whiteKeyHeight,
  whiteKeys,
  whiteKeyWidth,
} from './keys';

const keyGap = 1.5;

const palette = {
  ivory: '#f6f5f0',
  ivoryPressed: '#ffc9c6',
  whiteLip: '#d8d6ce',
  whiteLipPressed: '#e9a19d',
  ebony: '#0f1016',
  ebonyFace: '#2a2c37',
  ebonyFacePressed: '#ff7774',
  keySlip: '#11131c',
  felt: '#7d2633',
  label: '#9ba1b9',
};

interface KeyProps {
  pianoKey: PianoKey;
  isPressed: boolean;
}

const WhiteKey = memo(({ pianoKey, isPressed }: KeyProps) => (
  <View
    accessible
    accessibilityRole="button"
    accessibilityLabel={pianoKey.name}
    style={[
      styles.whiteKey,
      { left: pianoKey.x },
      isPressed && styles.whiteKeyPressed,
    ]}
  >
    <View
      style={[styles.whiteKeyFace, isPressed && styles.whiteKeyFacePressed]}
    >
      <View style={styles.fallboardShadow} />
      <View style={styles.fallboardShadowSoft} />
      {pianoKey.midi % 12 === 0 && (
        <Text style={styles.label}>{pianoKey.name}</Text>
      )}
    </View>
  </View>
));

const BlackKey = memo(({ pianoKey, isPressed }: KeyProps) => (
  <View
    accessible
    accessibilityRole="button"
    accessibilityLabel={pianoKey.name}
    style={[styles.blackKey, { left: pianoKey.x }]}
  >
    <View
      style={[styles.blackKeyFace, isPressed && styles.blackKeyFacePressed]}
    />
  </View>
));

interface KeyboardProps {
  scrollRef: AnimatedRef<Animated.ScrollView>;
  pressedKeys: ReadonlySet<number>;
  onNoteOn: (midi: number) => void;
  onNoteOff: (midi: number) => void;
  onViewportWidthChange: (width: number) => void;
}

/**
 * The scrollable 88-key keyboard. A single multi-touch gesture hit-tests every
 * finger against the key layout, so chords and slides across keys work. The
 * keys themselves are accessibility elements only. Dragging on the keys plays
 * them, so the keyboard is scrolled from outside (overview strip, octave
 * menu).
 */
const Keyboard: React.FC<KeyboardProps> = ({
  scrollRef,
  pressedKeys,
  onNoteOn,
  onNoteOff,
  onViewportWidthChange,
}) => {
  const keysByTouch = useSharedValue<Record<number, number>>({});

  const pressTouches = (event: GestureTouchEvent) => {
    'worklet';
    const next = { ...keysByTouch.value };

    for (const touch of event.changedTouches) {
      const midi = keyAt(touch.x, touch.y);
      const previous = next[touch.id];

      if (midi === previous) {
        continue;
      }

      if (previous !== undefined && previous >= 0) {
        scheduleOnRN(onNoteOff, previous);
      }
      if (midi >= 0) {
        scheduleOnRN(onNoteOn, midi);
      }
      next[touch.id] = midi;
    }

    keysByTouch.value = next;
  };

  const releaseTouches = (event: GestureTouchEvent) => {
    'worklet';
    const next = { ...keysByTouch.value };

    for (const touch of event.changedTouches) {
      const midi = next[touch.id];
      if (midi !== undefined && midi >= 0) {
        scheduleOnRN(onNoteOff, midi);
      }
      delete next[touch.id];
    }

    keysByTouch.value = next;

    if (Object.keys(next).length === 0) {
      GestureStateManager.deactivate(event.handlerTag);
    }
  };

  const touches = useManualGesture({
    onTouchesDown: (event) => {
      GestureStateManager.activate(event.handlerTag);
      pressTouches(event);
    },
    onTouchesMove: pressTouches,
    onTouchesUp: releaseTouches,
    onTouchesCancel: releaseTouches,
  });

  const onLayout = (event: LayoutChangeEvent) => {
    onViewportWidthChange(event.nativeEvent.layout.width);
  };

  return (
    <View>
      <View style={styles.keySlip}>
        <View style={styles.felt} />
      </View>
      <Animated.ScrollView
        ref={scrollRef}
        horizontal
        scrollEnabled={false}
        showsHorizontalScrollIndicator={false}
        onLayout={onLayout}
      >
        <GestureDetector gesture={touches}>
          <View style={styles.keys}>
            {whiteKeys.map((key) => (
              <WhiteKey
                key={key.midi}
                pianoKey={key}
                isPressed={pressedKeys.has(key.midi)}
              />
            ))}
            {blackKeys.map((key) => (
              <BlackKey
                key={key.midi}
                pianoKey={key}
                isPressed={pressedKeys.has(key.midi)}
              />
            ))}
          </View>
        </GestureDetector>
      </Animated.ScrollView>
    </View>
  );
};

export default Keyboard;

const styles = StyleSheet.create({
  keySlip: {
    height: 18,
    justifyContent: 'flex-end',
    backgroundColor: palette.keySlip,
  },
  felt: {
    height: 3,
    backgroundColor: palette.felt,
  },
  keys: {
    width: keyboardWidth,
    height: whiteKeyHeight,
    backgroundColor: palette.ebony,
  },
  whiteKey: {
    position: 'absolute',
    top: 0,
    width: whiteKeyWidth - keyGap,
    height: whiteKeyHeight,
    borderBottomLeftRadius: 6,
    borderBottomRightRadius: 6,
    backgroundColor: palette.whiteLip,
  },
  whiteKeyPressed: {
    backgroundColor: palette.whiteLipPressed,
  },
  /** The key top; the body shows below it as the rounded front edge. */
  whiteKeyFace: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 9,
    overflow: 'hidden',
    alignItems: 'center',
    justifyContent: 'flex-end',
    borderBottomLeftRadius: 5,
    borderBottomRightRadius: 5,
    backgroundColor: palette.ivory,
  },
  whiteKeyFacePressed: {
    bottom: 3,
    backgroundColor: palette.ivoryPressed,
  },
  fallboardShadow: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    height: 6,
    backgroundColor: 'rgba(0, 0, 0, 0.22)',
  },
  fallboardShadowSoft: {
    position: 'absolute',
    top: 6,
    left: 0,
    right: 0,
    height: 10,
    backgroundColor: 'rgba(0, 0, 0, 0.07)',
  },
  label: {
    fontSize: 10,
    fontWeight: '600',
    color: palette.label,
    marginBottom: 6,
  },
  blackKey: {
    position: 'absolute',
    top: 0,
    width: blackKeyWidth,
    height: blackKeyHeight,
    borderBottomLeftRadius: 4,
    borderBottomRightRadius: 4,
    backgroundColor: palette.ebony,
  },
  blackKeyFace: {
    position: 'absolute',
    top: 0,
    left: 3,
    right: 3,
    bottom: 12,
    borderBottomLeftRadius: 3,
    borderBottomRightRadius: 3,
    backgroundColor: palette.ebonyFace,
  },
  blackKeyFacePressed: {
    bottom: 5,
    backgroundColor: palette.ebonyFacePressed,
  },
});
