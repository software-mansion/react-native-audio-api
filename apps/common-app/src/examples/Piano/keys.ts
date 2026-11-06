/** Full 88-key piano range, A0 (MIDI 21) to C8 (MIDI 108). */
export const lowestMidi = 21;
export const highestMidi = 108;

export const whiteKeyWidth = 44;
export const whiteKeyHeight = 236;
export const blackKeyWidth = whiteKeyWidth * 0.58;
export const blackKeyHeight = whiteKeyHeight * 0.62;

const noteClassNames = [
  'C',
  'C#',
  'D',
  'D#',
  'E',
  'F',
  'F#',
  'G',
  'G#',
  'A',
  'A#',
  'B',
];

/**
 * Black keys on a real piano are not centred on the gap between white keys:
 * C#/D# and F#/A# lean away from each other. Offsets are fractions of a white
 * key width, keyed by note class.
 */
const blackKeyOffsets: Record<number, number> = {
  1: -0.08,
  3: 0.08,
  6: -0.1,
  8: 0,
  10: 0.1,
};

export interface PianoKey {
  midi: number;
  name: string;
  octave: number;
  isBlack: boolean;
  frequency: number;
  /** Left edge in keyboard content coordinates. */
  x: number;
}

function isBlackNote(midi: number) {
  return midi % 12 in blackKeyOffsets;
}

export function noteName(midi: number) {
  return `${noteClassNames[midi % 12]}${Math.floor(midi / 12) - 1}`;
}

export function noteFrequency(midi: number) {
  return 440 * Math.pow(2, (midi - 69) / 12);
}

function buildKeys(): PianoKey[] {
  const keys: PianoKey[] = [];
  let whiteIndex = 0;

  for (let midi = lowestMidi; midi <= highestMidi; midi += 1) {
    const isBlack = isBlackNote(midi);
    const x = isBlack
      ? whiteIndex * whiteKeyWidth -
        blackKeyWidth / 2 +
        blackKeyOffsets[midi % 12] * whiteKeyWidth
      : whiteIndex * whiteKeyWidth;

    keys.push({
      midi,
      name: noteName(midi),
      octave: Math.floor(midi / 12) - 1,
      isBlack,
      frequency: noteFrequency(midi),
      x,
    });

    if (!isBlack) {
      whiteIndex += 1;
    }
  }

  return keys;
}

export const pianoKeys = buildKeys();
export const whiteKeys = pianoKeys.filter((key) => !key.isBlack);
export const blackKeys = pianoKeys.filter((key) => key.isBlack);
export const keyboardWidth = whiteKeys.length * whiteKeyWidth;

const whiteKeyMidis = whiteKeys.map((key) => key.midi);
const blackKeyBounds = blackKeys.map((key) => ({
  midi: key.midi,
  left: key.x,
  right: key.x + blackKeyWidth,
}));

/** MIDI note under a point in keyboard content coordinates, or -1 when outside. */
export function keyAt(x: number, y: number): number {
  'worklet';

  if (x < 0 || x >= keyboardWidth || y < 0 || y > whiteKeyHeight) {
    return -1;
  }

  if (y <= blackKeyHeight) {
    for (const bounds of blackKeyBounds) {
      if (x >= bounds.left && x <= bounds.right) {
        return bounds.midi;
      }
    }
  }

  return whiteKeyMidis[Math.floor(x / whiteKeyWidth)];
}

/**
 * Scroll offset that puts C of the given octave at the left edge (A0 for octave
 * 0).
 */
export function octaveScrollOffset(octave: number) {
  const target = octave === 0 ? lowestMidi : (octave + 1) * 12;
  const key = whiteKeys.find((whiteKey) => whiteKey.midi === target);
  return key ? key.x : 0;
}

/** Octave of the white key at the left edge of the viewport. */
export function octaveAtOffset(offset: number) {
  'worklet';
  const index = Math.min(
    whiteKeys.length - 1,
    Math.max(0, Math.round(offset / whiteKeyWidth))
  );
  return whiteKeys[index].octave;
}

export const octaves = [0, 1, 2, 3, 4, 5, 6, 7];
