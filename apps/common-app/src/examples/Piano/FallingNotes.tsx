import { BlurStyle, Canvas, Picture, Skia } from '@shopify/react-native-skia';
import React, { useMemo, useState } from 'react';
import { LayoutChangeEvent, StyleSheet, View } from 'react-native';
import { SharedValue, useDerivedValue } from 'react-native-reanimated';

import { colors } from '../../styles';
import { blackKeyWidth, pianoKeys, whiteKeyWidth } from './keys';
import type { ScoreNote } from './usePianoDevApi';

/** Falling speed in points per second: how far ahead of time a note appears. */
const fallSpeed = 210;
const barRadius = 5;
const middleC = 60;

const handColors = {
  left: colors.blue,
  right: colors.accent,
};

interface FallingNotesProps {
  score: ScoreNote[];
  /** Score time in seconds. */
  elapsed: SharedValue<number>;
  scrollOffset: SharedValue<number>;
}

/**
 * Synthesia-style bars falling onto the keys while the app plays a score. A bar
 * reaches the bottom edge, the top of the keyboard, the moment its note starts,
 * and its length is the note's duration. Bars follow the keyboard's scroll
 * offset, so they stay above their keys.
 */
const FallingNotes: React.FC<FallingNotesProps> = ({
  score,
  elapsed,
  scrollOffset,
}) => {
  const [size, setSize] = useState({ width: 0, height: 0 });

  const bars = useMemo(() => {
    const keyByMidi = new Map(pianoKeys.map((key) => [key.midi, key]));

    return score.flatMap((note) => {
      const key = keyByMidi.get(note.midi);
      if (!key) {
        return [];
      }

      const width = key.isBlack ? blackKeyWidth - 4 : whiteKeyWidth - 6;
      const keyWidth = key.isBlack ? blackKeyWidth : whiteKeyWidth - 1.5;
      const hand = note.hand ?? (note.midi >= middleC ? 'right' : 'left');

      return [
        {
          x: key.x + (keyWidth - width) / 2,
          width,
          start: note.time,
          end: note.time + note.duration,
          color: Skia.Color(handColors[hand]),
        },
      ];
    });
  }, [score]);

  const paints = useMemo(() => {
    const bar = Skia.Paint();
    const glow = Skia.Paint();
    glow.setMaskFilter(Skia.MaskFilter.MakeBlur(BlurStyle.Normal, 10, true));
    return { bar, glow };
  }, []);

  const picture = useDerivedValue(() => {
    const { width, height } = size;
    const recorder = Skia.PictureRecorder();
    const canvas = recorder.beginRecording(Skia.XYWHRect(0, 0, width, height));
    const now = elapsed.value;
    const lookAhead = height / fallSpeed;

    for (const bar of bars) {
      if (bar.start > now + lookAhead || bar.end < now) {
        continue;
      }

      const bottom = height - (bar.start - now) * fallSpeed;
      const top = height - (bar.end - now) * fallSpeed;
      const rect = Skia.RRectXY(
        Skia.XYWHRect(bar.x - scrollOffset.value, top, bar.width, bottom - top),
        barRadius,
        barRadius
      );

      const isSounding = bar.start <= now;
      if (isSounding) {
        paints.glow.setColor(bar.color);
        canvas.drawRRect(rect, paints.glow);
      }
      paints.bar.setColor(bar.color);
      paints.bar.setAlphaf(isSounding ? 1 : 0.85);
      canvas.drawRRect(rect, paints.bar);
    }

    return recorder.finishRecordingAsPicture();
  });

  const onLayout = (event: LayoutChangeEvent) => {
    const { width, height } = event.nativeEvent.layout;
    setSize({ width, height });
  };

  return (
    <View style={StyleSheet.absoluteFill} onLayout={onLayout}>
      <Canvas style={StyleSheet.absoluteFill}>
        <Picture picture={picture} />
      </Canvas>
    </View>
  );
};

export default FallingNotes;
