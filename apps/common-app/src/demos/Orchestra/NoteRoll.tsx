// What this phone plays, made readable on camera: a piano roll of its parts
// scrolling past a fixed playhead, with the notes it is sounding lit. Pitch
// runs up the screen one row per semitone, so leaps look like leaps.
import React, { FC, memo, useEffect, useMemo, useState } from 'react';
import { LayoutChangeEvent, StyleSheet, Text, View } from 'react-native';
import Animated, {
  cancelAnimation,
  Easing,
  SharedValue,
  useAnimatedStyle,
  useSharedValue,
  withSequence,
  withTiming,
} from 'react-native-reanimated';

import { colors } from '../../styles';
import {
  DRUM_PIECES,
  drumPieceFor,
  type DrumPiece,
} from '../../../orchestra-server/drumKit';
import type { NoteListener, PartNote } from './Scheduler';
import type { ScorePart } from './protocol';

const BEAT_WIDTH = 56;
const BEATS_PER_BAR = 4;
/** Where "now" sits, as a fraction of the roll's width. */
const PLAYHEAD_AT = 0.3;
const GUTTER_WIDTH = 52;
/** Labels keep this height however thin the rows get. */
const LABEL_HEIGHT = 12;
/** Even a staccato note stays lit long enough to be seen. */
const MIN_LIT_MS = 120;

const PAST_OPACITY = 0.25;
const UPCOMING_OPACITY = 0.6;

const NOTE_NAMES = [
  'C',
  'C♯',
  'D',
  'D♯',
  'E',
  'F',
  'F♯',
  'G',
  'G♯',
  'A',
  'A♯',
  'B',
];
export function noteName(midi: number): string {
  return `${NOTE_NAMES[midi % 12]}${Math.floor(midi / 12) - 1}`;
}

interface NoteRollProps {
  notes: readonly PartNote[];
  parts: readonly ScorePart[];
  lengthBeats: number;
  bpm: number;
  playing: boolean;
  /** Changes on every transport start. */
  epoch: number;
  subscribeToBeats: (listener: (beat: number) => void) => () => void;
  subscribeToNotes: (listener: NoteListener) => () => void;
}

const NoteRoll: FC<NoteRollProps> = ({
  notes,
  parts,
  lengthBeats,
  bpm,
  playing,
  epoch,
  subscribeToBeats,
  subscribeToNotes,
}) => {
  const [size, setSize] = useState({ width: 0, height: 0 });
  const [beat, setBeat] = useState(0);
  const [lit, setLit] = useState<ReadonlySet<string>>(new Set());
  const position = useSharedValue(0);
  const msPerBeat = 60000 / bpm;

  // Every transport start begins from the top of the score.
  useEffect(() => {
    cancelAnimation(position);
    position.value = 0;
    setBeat(0);
    setLit(new Set());
  }, [epoch, position]);

  useEffect(() => {
    if (!playing) {
      cancelAnimation(position);
    }
  }, [playing, position]);

  // Each beat glides the roll to the next one; a phone that joined late, or a
  // late timer, snaps first so the playhead never drifts off the music.
  useEffect(
    () =>
      subscribeToBeats((heardBeat) => {
        setBeat(heardBeat);
        const glide = withTiming(heardBeat + 1, {
          duration: msPerBeat,
          easing: Easing.linear,
        });
        position.value =
          Math.abs(position.value - heardBeat) > 0.5
            ? withSequence(withTiming(heardBeat, { duration: 0 }), glide)
            : glide;
      }),
    [subscribeToBeats, msPerBeat, position]
  );

  useEffect(() => {
    const timers = new Set<ReturnType<typeof setTimeout>>();
    const unsubscribe = subscribeToNotes((cue) => {
      setLit((current) => new Set(current).add(cue.key));
      const timer = setTimeout(
        () => {
          timers.delete(timer);
          setLit((current) => {
            const next = new Set(current);
            next.delete(cue.key);
            return next;
          });
        },
        Math.max(MIN_LIT_MS, cue.durationMs)
      );
      timers.add(timer);
    });
    return () => {
      unsubscribe();
      timers.forEach(clearTimeout);
    };
  }, [subscribeToNotes]);

  const onLayout = (event: LayoutChangeEvent) => {
    const { width, height } = event.nativeEvent.layout;
    setSize({ width, height });
  };

  const rollWidth = Math.max(0, size.width - GUTTER_WIDTH);
  const playheadX = rollWidth * PLAYHEAD_AT;
  const scrollStyle = useAnimatedStyle(() => ({
    transform: [{ translateX: playheadX - position.value * BEAT_WIDTH }],
  }));

  const grid = useMemo(() => gridFor(notes), [notes]);
  const rowHeight = grid ? size.height / grid.rows.length : 0;
  const rowTop = (midi: number) => (grid?.rowOf.get(midi) ?? 0) * rowHeight;

  const firstBeat = Math.floor(beat - playheadX / BEAT_WIDTH) - 1;
  const lastBeat = Math.ceil(beat + (rollWidth - playheadX) / BEAT_WIDTH) + 2;
  const visible = useMemo(
    () => notesBetween(notes, lengthBeats, firstBeat, lastBeat),
    [notes, lengthBeats, firstBeat, lastBeat]
  );

  return (
    <View style={styles.container}>
      <View style={styles.roll} onLayout={onLayout}>
        {grid && size.height > 0 && (
          <>
            <RowGrid
              rows={grid.rows}
              rowHeight={rowHeight}
              width={size.width}
            />
            <View style={styles.window}>
              <Animated.View style={[styles.scroller, scrollStyle]}>
                {beatLines(firstBeat, lastBeat, size.height)}
                {visible.map(({ note, key, startBeat }) => (
                  <NoteBar
                    key={key}
                    left={startBeat * BEAT_WIDTH}
                    width={Math.max(4, note.duration * BEAT_WIDTH - 2)}
                    top={rowTop(note.midi) + 1}
                    height={Math.max(3, rowHeight - 2)}
                    color={parts[note.partIndex]?.color ?? colors.main}
                    endBeat={startBeat + note.duration}
                    isLit={lit.has(key)}
                    position={position}
                  />
                ))}
              </Animated.View>
              <View
                pointerEvents="none"
                style={[styles.playhead, { left: playheadX }]}
              />
            </View>
          </>
        )}
        {!grid && <Text style={styles.empty}>no notes in this part</Text>}
      </View>
    </View>
  );
};

interface NoteBarProps {
  left: number;
  width: number;
  top: number;
  height: number;
  color: string;
  endBeat: number;
  isLit: boolean;
  position: SharedValue<number>;
}

// Dims on the UI thread the frame its end crosses the playhead, rather than
// on the next beat cue.
const NoteBar = memo<NoteBarProps>(function NoteBar({
  left,
  width,
  top,
  height,
  color,
  endBeat,
  isLit,
  position,
}) {
  const dimStyle = useAnimatedStyle(() => ({
    opacity: isLit
      ? 1
      : position.value >= endBeat
        ? PAST_OPACITY
        : UPCOMING_OPACITY,
  }));
  return (
    <Animated.View
      style={[
        styles.note,
        { left, width, top, height, backgroundColor: color },
        isLit && styles.litNote,
        dimStyle,
      ]}
    />
  );
});

// Faint rows, labelled on the left: semitones (stronger on every C, black
// keys shaded) so intervals can be read off the grid, or one row per drum.
const RowGrid = memo<{
  rows: readonly GridRow[];
  rowHeight: number;
  width: number;
}>(function RowGrid({ rows, rowHeight, width }) {
  // Every label fits once rows are as tall as the text; thinner rows label
  // only their octaves (Cs), which sit twelve rows apart, centred on the row
  // and spilling over its neighbours.
  const labelEveryRow = rowHeight >= LABEL_HEIGHT;
  const height = rows.length * rowHeight;
  return (
    <View style={StyleSheet.absoluteFill}>
      {rows.map((row, index) => (
        <View
          key={row.id}
          style={[
            styles.row,
            { top: index * rowHeight, height: rowHeight, width },
            row.shade === 'blackKey' && styles.blackKeyRow,
            row.shade === 'octave' && styles.cRow,
          ]}
        />
      ))}
      {rows.map((row, index) =>
        row.label !== null && (labelEveryRow || row.shade === 'octave') ? (
          <Text
            key={`label-${row.id}`}
            style={[
              styles.rowLabel,
              {
                top: Math.min(
                  height - LABEL_HEIGHT,
                  Math.max(0, (index + 0.5) * rowHeight - LABEL_HEIGHT / 2)
                ),
              },
            ]}
            numberOfLines={1}
          >
            {row.label}
          </Text>
        ) : null
      )}
    </View>
  );
});

function beatLines(firstBeat: number, lastBeat: number, height: number) {
  const lines = [];
  for (let beat = Math.max(0, firstBeat); beat <= lastBeat; beat += 1) {
    const isBar = beat % BEATS_PER_BAR === 0;
    lines.push(
      <View
        key={`beat-${beat}`}
        style={[
          styles.beatLine,
          { left: beat * BEAT_WIDTH, height },
          isBar && styles.barLine,
        ]}
      >
        {isBar && (
          <Text style={styles.barNumber}>{beat / BEATS_PER_BAR + 1}</Text>
        )}
      </View>
    );
  }
  return lines;
}

interface GridRow {
  id: string;
  label: string | null;
  shade?: 'blackKey' | 'octave';
}

interface Grid {
  /** Top to bottom. */
  rows: GridRow[];
  /** Row index of each midi value (a drum piece's keys share one row). */
  rowOf: Map<number, number>;
}

// Cymbals on top, kick at the bottom, like a drum chart.
export const DRUM_ROW_ORDER: readonly DrumPiece[] = [
  'crash',
  'ride',
  'openHat',
  'closedHat',
  'highTom',
  'lowTom',
  'clap',
  'snare',
  'kick',
];

function gridFor(notes: readonly PartNote[]): Grid | null {
  if (notes.length === 0) {
    return null;
  }
  return notes.every((note) => note.timbre === 'kit')
    ? drumGrid(notes)
    : pitchGrid(notes);
}

function pitchGrid(notes: readonly PartNote[]): Grid {
  let low = Number.POSITIVE_INFINITY;
  let high = Number.NEGATIVE_INFINITY;
  for (const note of notes) {
    low = Math.min(low, note.midi - 1);
    high = Math.max(high, note.midi + 1);
  }
  const labelOctavesOnly = high - low > 24;
  const rows: GridRow[] = [];
  const rowOf = new Map<number, number>();
  for (let midi = high; midi >= low; midi -= 1) {
    const isC = midi % 12 === 0;
    rowOf.set(midi, rows.length);
    rows.push({
      id: `pitch-${midi}`,
      label: !labelOctavesOnly || isC ? noteName(midi) : null,
      shade: isC
        ? 'octave'
        : [1, 3, 6, 8, 10].includes(midi % 12)
          ? 'blackKey'
          : undefined,
    });
  }
  return { rows, rowOf };
}

function drumGrid(notes: readonly PartNote[]): Grid {
  const used = new Set(
    notes.map((note) => drumPieceFor(note.midi)).filter((piece) => piece)
  );
  const pieces = DRUM_ROW_ORDER.filter((piece) => used.has(piece));
  const rowOf = new Map<number, number>();
  pieces.forEach((piece, index) => {
    for (const key of DRUM_PIECES[piece].keys) {
      rowOf.set(key, index);
    }
  });
  return {
    rows: pieces.map((piece) => ({
      id: `drum-${piece}`,
      label: DRUM_PIECES[piece].label,
    })),
    rowOf,
  };
}

// The notes that overlap [firstBeat, lastBeat] on the transport's beat line,
// across as many loops of the score as the window spans.
function notesBetween(
  notes: readonly PartNote[],
  lengthBeats: number,
  firstBeat: number,
  lastBeat: number
) {
  const visible: { note: PartNote; key: string; startBeat: number }[] = [];
  if (lengthBeats <= 0) {
    return visible;
  }
  const firstCycle = Math.max(0, Math.floor(firstBeat / lengthBeats) - 1);
  const lastCycle = Math.floor(lastBeat / lengthBeats);
  for (let cycle = firstCycle; cycle <= lastCycle; cycle += 1) {
    notes.forEach((note, index) => {
      const startBeat = cycle * lengthBeats + note.beat;
      if (startBeat <= lastBeat && startBeat + note.duration >= firstBeat) {
        visible.push({ note, key: `${cycle}:${index}`, startBeat });
      }
    });
  }
  return visible;
}

export default NoteRoll;

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  roll: {
    flex: 1,
    borderRadius: 8,
    overflow: 'hidden',
    backgroundColor: colors.backgroundDark,
  },
  window: {
    position: 'absolute',
    top: 0,
    right: 0,
    bottom: 0,
    left: GUTTER_WIDTH,
    overflow: 'hidden',
  },
  scroller: {
    position: 'absolute',
    top: 0,
    right: 0,
    bottom: 0,
    left: 0,
  },
  row: {
    position: 'absolute',
    left: 0,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: '#ffffff14',
  },
  blackKeyRow: {
    backgroundColor: '#00000033',
  },
  cRow: {
    borderBottomColor: '#ffffff40',
  },
  rowLabel: {
    position: 'absolute',
    left: 0,
    width: GUTTER_WIDTH - 4,
    height: LABEL_HEIGHT,
    lineHeight: LABEL_HEIGHT,
    fontSize: 10,
    color: colors.white,
    opacity: 0.55,
    textAlign: 'right',
  },
  beatLine: {
    position: 'absolute',
    top: 0,
    width: StyleSheet.hairlineWidth,
    backgroundColor: '#ffffff1a',
  },
  barLine: {
    width: 1,
    backgroundColor: '#ffffff40',
  },
  barNumber: {
    position: 'absolute',
    top: 2,
    left: 4,
    width: 30,
    color: colors.white,
    opacity: 0.5,
    fontSize: 10,
  },
  note: {
    position: 'absolute',
    borderRadius: 3,
  },
  litNote: {
    borderWidth: 1,
    borderColor: colors.white,
  },
  playhead: {
    position: 'absolute',
    top: 0,
    bottom: 0,
    width: 2,
    backgroundColor: colors.white,
    opacity: 0.9,
  },
  empty: {
    color: colors.white,
    opacity: 0.5,
    textAlign: 'center',
    marginTop: 24,
  },
});
