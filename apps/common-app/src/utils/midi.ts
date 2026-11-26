/** Full 88-key piano range, A0 (MIDI 21) to C8 (MIDI 108). */
export const lowestPianoMidi = 21;
export const highestPianoMidi = 108;

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

export function noteName(midi: number) {
  return `${noteClassNames[midi % 12]}${Math.floor(midi / 12) - 1}`;
}

export function noteFrequency(midi: number) {
  return 440 * Math.pow(2, (midi - 69) / 12);
}
