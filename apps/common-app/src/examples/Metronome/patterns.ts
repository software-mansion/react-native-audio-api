import type { Pattern } from '../../types';

const oneBeatPerBar: Pattern[] = [
  {
    instrumentName: 'downbeat',
    steps: [true],
  },
  {
    instrumentName: 'regularbeat',
    steps: [false],
  },
];

const twoBeatsPerBar: Pattern[] = [
  {
    instrumentName: 'downbeat',
    steps: [true, false],
  },
  {
    instrumentName: 'regularbeat',
    steps: [false, true],
  },
];

const threeBeatsPerBar: Pattern[] = [
  {
    instrumentName: 'downbeat',
    steps: [true, false, false],
  },
  {
    instrumentName: 'regularbeat',
    steps: [false, true, true],
  },
];

const fourBeatsPerBar: Pattern[] = [
  {
    instrumentName: 'downbeat',
    steps: [true, false, false, false],
  },
  {
    instrumentName: 'regularbeat',
    steps: [false, true, true, true],
  },
];

const fiveBeatsPerBar: Pattern[] = [
  {
    instrumentName: 'downbeat',
    steps: [true, false, false, false, false],
  },
  {
    instrumentName: 'regularbeat',
    steps: [false, true, true, true, true],
  },
];

const sixBeatsPerBar: Pattern[] = [
  {
    instrumentName: 'downbeat',
    steps: [true, false, false, false, false, false],
  },
  {
    instrumentName: 'regularbeat',
    steps: [false, true, true, true, true, true],
  },
];

const sevenBeatsPerBar: Pattern[] = [
  {
    instrumentName: 'downbeat',
    steps: [true, false, false, false, false, false, false],
  },
  {
    instrumentName: 'regularbeat',
    steps: [false, true, true, true, true, true, true],
  },
];

const eightBeatsPerBar: Pattern[] = [
  {
    instrumentName: 'downbeat',
    steps: [true, false, false, false, false, false, false, false],
  },
  {
    instrumentName: 'regularbeat',
    steps: [false, true, true, true, true, true, true, true],
  },
];

export const patterns: Record<number, Pattern[]> = {
  1: oneBeatPerBar,
  2: twoBeatsPerBar,
  3: threeBeatsPerBar,
  4: fourBeatsPerBar,
  5: fiveBeatsPerBar,
  6: sixBeatsPerBar,
  7: sevenBeatsPerBar,
  8: eightBeatsPerBar,
};

export const defaultPattern = 4;
