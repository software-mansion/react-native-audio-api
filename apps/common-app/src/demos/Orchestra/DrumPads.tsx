import React, { FC, useEffect, useMemo } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import Animated, {
  useAnimatedStyle,
  useSharedValue,
  withSequence,
  withTiming,
} from 'react-native-reanimated';

import { colors, layout } from '../../styles';
import {
  DRUM_PIECES,
  drumPieceFor,
  type DrumPiece,
} from '../../../orchestra-server/drumKit';
import { DRUM_ROW_ORDER } from './NoteRoll';
import type { NoteListener, PartNote } from './Scheduler';
import type { ScorePart } from './protocol';

interface DrumPadsProps {
  notes: readonly PartNote[];
  parts: readonly ScorePart[];
  subscribeToNotes: (listener: NoteListener) => () => void;
}

const PAD_HEIGHT = 56;
const FADE_MS = 160;
const POP_SCALE = 1.08;

/** One pad per kit piece this phone plays, lit on every hit; kick first. */
const DrumPads: FC<DrumPadsProps> = ({ notes, parts, subscribeToNotes }) => {
  const pads = useMemo(() => {
    const colorOf = new Map<DrumPiece, string>();
    for (const note of notes) {
      const piece = note.timbre === 'kit' ? drumPieceFor(note.midi) : null;
      if (piece && !colorOf.has(piece)) {
        colorOf.set(piece, parts[note.partIndex]?.color ?? colors.main);
      }
    }
    return [...DRUM_ROW_ORDER]
      .reverse()
      .filter((piece) => colorOf.has(piece))
      .map((piece) => ({ piece, color: colorOf.get(piece)! }));
  }, [notes, parts]);

  if (pads.length === 0) {
    return null;
  }
  return (
    <View style={styles.row}>
      {pads.map(({ piece, color }) => (
        <Pad
          key={piece}
          piece={piece}
          color={color}
          subscribeToNotes={subscribeToNotes}
        />
      ))}
    </View>
  );
};

interface PadProps {
  piece: DrumPiece;
  color: string;
  subscribeToNotes: (listener: NoteListener) => () => void;
}

const Pad: FC<PadProps> = ({ piece, color, subscribeToNotes }) => {
  const glow = useSharedValue(0);

  useEffect(
    () =>
      subscribeToNotes((cue) => {
        if (cue.timbre === 'kit' && drumPieceFor(cue.midi) === piece) {
          glow.value = withSequence(
            withTiming(1, { duration: 0 }),
            withTiming(0, { duration: FADE_MS })
          );
        }
      }),
    [subscribeToNotes, piece, glow]
  );

  const litStyle = useAnimatedStyle(() => ({ opacity: glow.value }));
  const popStyle = useAnimatedStyle(() => ({
    transform: [{ scale: 1 + (POP_SCALE - 1) * glow.value }],
  }));

  return (
    <Animated.View style={[styles.pad, { borderColor: color }, popStyle]}>
      <Animated.View
        style={[StyleSheet.absoluteFill, { backgroundColor: color }, litStyle]}
      />
      <Text style={styles.label} numberOfLines={1}>
        {DRUM_PIECES[piece].label}
      </Text>
    </Animated.View>
  );
};

export default DrumPads;

const styles = StyleSheet.create({
  row: {
    flexDirection: 'row',
    gap: layout.spacing,
  },
  pad: {
    flex: 1,
    height: PAD_HEIGHT,
    borderRadius: layout.radius,
    borderWidth: 2,
    backgroundColor: colors.backgroundLight,
    alignItems: 'center',
    justifyContent: 'center',
    overflow: 'hidden',
  },
  label: {
    color: colors.white,
    fontSize: 13,
    fontWeight: '600',
  },
});
