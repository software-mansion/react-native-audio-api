// The conductor's scores: the built-in "Relay", plus every MIDI file in ./midi.
// Scores are sent to every phone in the `state` message, so the server is the
// single source of truth for what gets played. Phone speakers reproduce almost
// nothing below ~300 Hz, so Relay keeps every part at midi 55 or above.
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

import { loadMidiFolder } from './midi.ts';
import { SampleStore } from './samples.ts';
import { DRUMS } from './drumKit.ts';
import type {
  Score,
  ScoreNote,
  ScorePart,
} from '../src/demos/Orchestra/protocol.ts';

const BEATS_PER_BAR = 4;

// "Relay": six parts in D minor. Intro → theme → a dialogue where one motif
// hops from phone to phone → a build over a rising progression → the theme
// again → a held final chord, whose last bar sits on the dominant (A) so the
// loop falls back into the intro.

/** [beat offset within the bar, midi, duration in beats] */
type Figure = readonly [number, number, number];

interface Harmony {
  /** Voice-led three-note pad chord around middle C. */
  pad: readonly number[];
  /** Kept at midi 55+ so phone speakers can still carry it. */
  bass: number;
  /** The four-note dialogue motif spelled over this chord. */
  motif: readonly number[];
  counter: readonly Figure[];
}

const D_MINOR: Harmony = {
  pad: [62, 65, 69],
  bass: 62,
  motif: [69, 74, 76, 77],
  counter: [
    [0, 65, 2],
    [2, 69, 2],
  ],
};
const B_FLAT: Harmony = {
  pad: [62, 65, 70],
  bass: 58,
  motif: [65, 70, 72, 74],
  counter: [
    [0, 70, 2],
    [2, 65, 2],
  ],
};
const F_MAJOR: Harmony = {
  pad: [60, 65, 69],
  bass: 65,
  motif: [72, 77, 79, 81],
  counter: [
    [0, 69, 1.5],
    [1.5, 67, 0.5],
    [2, 65, 2],
  ],
};
const C_MAJOR: Harmony = {
  pad: [60, 64, 67],
  bass: 60,
  motif: [67, 72, 74, 76],
  counter: [
    [0, 64, 2],
    [2, 67, 1],
    [3, 64, 1],
  ],
};
const G_MINOR: Harmony = {
  pad: [62, 67, 70],
  bass: 55,
  motif: [67, 70, 72, 74],
  counter: [
    [0, 70, 2],
    [2, 74, 2],
  ],
};
const A_MAJOR: Harmony = {
  pad: [61, 64, 69],
  bass: 57,
  motif: [64, 69, 71, 73],
  counter: [
    [0, 73, 2],
    [2, 76, 2],
  ],
};

// i – VI – III – VII, and the build's iv – VI – VII – V that pulls back home.
const HOME = [D_MINOR, B_FLAT, F_MAJOR, C_MAJOR];
const RISE = [G_MINOR, B_FLAT, C_MAJOR, A_MAJOR];

type Section = 'intro' | 'theme' | 'dialogue' | 'build' | 'reprise' | 'coda';

const FORM: readonly (readonly [Section, readonly Harmony[]])[] = [
  ['intro', HOME],
  ['theme', [...HOME, ...HOME]],
  ['dialogue', [...HOME, ...HOME]],
  ['build', [...RISE, ...RISE]],
  ['reprise', [...HOME, ...HOME]],
  ['coda', [D_MINOR, D_MINOR, D_MINOR, A_MAJOR]],
];

interface Bar {
  start: number;
  section: Section;
  barInSection: number;
  harmony: Harmony;
}

function relayBars(): Bar[] {
  const bars: Bar[] = [];
  for (const [section, harmonies] of FORM) {
    harmonies.forEach((harmony, barInSection) => {
      bars.push({
        start: bars.length * BEATS_PER_BAR,
        section,
        barInSection,
        harmony,
      });
    });
  }
  return bars;
}

// One phrase per bar of the theme; the motif (A D E F) opens bars 1, 3 and 5.
const THEME_MELODY: readonly (readonly Figure[])[] = [
  [
    [0, 69, 0.5],
    [0.5, 74, 0.5],
    [1, 76, 0.5],
    [1.5, 77, 1.5],
    [3, 76, 0.5],
    [3.5, 74, 0.5],
  ],
  [
    [0, 74, 1.5],
    [1.5, 72, 0.5],
    [2, 70, 1],
    [3, 69, 1],
  ],
  [
    [0, 72, 0.5],
    [0.5, 77, 0.5],
    [1, 79, 0.5],
    [1.5, 81, 1.5],
    [3, 79, 0.5],
    [3.5, 77, 0.5],
  ],
  [
    [0, 76, 2],
    [2, 79, 1],
    [3, 76, 1],
  ],
  [
    [0, 69, 0.5],
    [0.5, 74, 0.5],
    [1, 76, 0.5],
    [1.5, 77, 1],
    [2.5, 81, 0.5],
    [3, 79, 0.5],
    [3.5, 77, 0.5],
  ],
  [
    [0, 74, 1],
    [1, 77, 0.5],
    [1.5, 74, 0.5],
    [2, 70, 2],
  ],
  [
    [0, 72, 1],
    [1, 69, 0.5],
    [1.5, 72, 0.5],
    [2, 77, 1],
    [3, 76, 0.5],
    [3.5, 77, 0.5],
  ],
  [
    [0, 76, 3],
    [3, 72, 1],
  ],
];

const BUILD_MELODY: readonly (readonly Figure[])[] = [
  [
    [0, 67, 0.5],
    [0.5, 70, 0.5],
    [1, 72, 0.5],
    [1.5, 74, 2.5],
  ],
  [
    [0, 70, 0.5],
    [0.5, 72, 0.5],
    [1, 74, 0.5],
    [1.5, 77, 2.5],
  ],
  [
    [0, 72, 0.5],
    [0.5, 76, 0.5],
    [1, 79, 3],
  ],
  [
    [0, 76, 0.5],
    [0.5, 81, 3.5],
  ],
  [[0, 74, 4]],
  [[0, 77, 4]],
  [[0, 79, 4]],
  [
    [0, 81, 2],
    [2, 79, 1],
    [3, 76, 0.5],
    [3.5, 73, 0.5],
  ],
];

type DialogueVoice = 'lead' | 'counter' | 'arpeggio' | 'bass';

const DIALOGUE_ORDER: readonly DialogueVoice[] = [
  'lead',
  'counter',
  'arpeggio',
  'bass',
];

function dialogueRegister(voice: DialogueVoice, motif: readonly number[]) {
  switch (voice) {
    case 'counter':
      return motif.map((midi) => midi + 12);
    case 'bass': {
      const octavesDown = Math.floor((Math.min(...motif) - 55) / 12);
      return motif.map((midi) => midi - 12 * octavesDown);
    }
    default:
      return motif;
  }
}

// The motif changes hands every bar, then every half bar, and in the last bar
// all four voices that carried it sound it together.
function dialogueTurn(voice: DialogueVoice, bar: Bar): ScoreNote[] {
  const motif = dialogueRegister(voice, bar.harmony.motif);
  const turn = DIALOGUE_ORDER.indexOf(voice);
  const isLastBar = bar.barInSection === 7;

  if (bar.barInSection < 4 || isLastBar) {
    if (!isLastBar && bar.barInSection !== turn) {
      return [];
    }
    return figureNotes(
      bar.start,
      [
        [0, motif[0], 0.5],
        [0.5, motif[1], 0.5],
        [1, motif[2], 0.5],
        [1.5, motif[3], 2],
      ],
      isLastBar ? 0.95 : 0.85
    );
  }

  const notes: ScoreNote[] = [];
  for (const half of [0, 1]) {
    const handoff = (bar.barInSection - 4) * 2 + half;
    if (handoff % DIALOGUE_ORDER.length === turn) {
      notes.push(
        ...figureNotes(
          bar.start + half * 2,
          [
            [0, motif[0], 0.5],
            [0.5, motif[1], 0.5],
            [1, motif[2], 0.5],
            [1.5, motif[3], 0.5],
          ],
          0.85
        )
      );
    }
  }
  return notes;
}

/** Downbeats get a little accent so phrases breathe. */
function figureNotes(
  barStart: number,
  figures: readonly Figure[],
  velocity: number
): ScoreNote[] {
  return figures.map(([offset, midi, duration]) => ({
    beat: barStart + offset,
    midi,
    duration,
    velocity: Math.min(
      1,
      Number.isInteger(offset) ? velocity + 0.08 : velocity
    ),
  }));
}

/** 0 → 1 across a section, for crescendos. */
function progress(bar: Bar, barsInSection: number): number {
  return bar.barInSection / (barsInSection - 1);
}

function leadPart(bars: Bar[]): ScoreNote[] {
  return bars.flatMap((bar) => {
    switch (bar.section) {
      case 'theme':
        return figureNotes(bar.start, THEME_MELODY[bar.barInSection], 0.75);
      case 'dialogue':
        return dialogueTurn('lead', bar);
      case 'build':
        return figureNotes(
          bar.start,
          BUILD_MELODY[bar.barInSection],
          0.65 + 0.3 * progress(bar, 8)
        );
      case 'reprise':
        return figureNotes(bar.start, THEME_MELODY[bar.barInSection], 0.9);
      case 'coda':
        return bar.barInSection === 0
          ? figureNotes(bar.start, [[0, 74, 8]], 0.85)
          : [];
      default:
        return [];
    }
  });
}

function counterPart(bars: Bar[]): ScoreNote[] {
  return bars.flatMap((bar) => {
    switch (bar.section) {
      case 'theme':
        return bar.barInSection >= 4
          ? figureNotes(bar.start, bar.harmony.counter, 0.6)
          : [];
      case 'dialogue':
        return dialogueTurn('counter', bar);
      case 'build':
        return bar.barInSection >= 4
          ? figureNotes(bar.start, bar.harmony.counter, 0.7)
          : [];
      case 'reprise':
        return figureNotes(bar.start, bar.harmony.counter, 0.7);
      case 'coda':
        return bar.barInSection === 0
          ? figureNotes(
              bar.start,
              [
                [0, 81, 2],
                [2, 77, 2],
              ],
              0.7
            )
          : [];
      default:
        return [];
    }
  });
}

function padPart(bars: Bar[]): ScoreNote[] {
  const chord = (bar: Bar, duration: number, velocity: number) =>
    bar.harmony.pad.map((midi) => ({
      beat: bar.start,
      midi,
      duration,
      velocity,
    }));

  return bars.flatMap((bar) => {
    switch (bar.section) {
      case 'intro':
        return chord(bar, 4, 0.45 + 0.1 * progress(bar, 4));
      case 'dialogue':
        return chord(bar, 4, 0.35);
      case 'build':
        return chord(bar, 4, 0.45 + 0.3 * progress(bar, 8));
      case 'coda':
        if (bar.barInSection === 0) {
          return chord(bar, 12, 0.6);
        }
        return bar.barInSection === 3 ? chord(bar, 4, 0.4) : [];
      default:
        return chord(bar, 4, 0.5);
    }
  });
}

function arpeggioPart(bars: Bar[]): ScoreNote[] {
  const pattern = [0, 1, 2, 3, 2, 1, 2, 1];
  const run = (bar: Bar, stepBeats: number, velocity: number) => {
    const [root, third, fifth] = bar.harmony.pad.map((midi) => midi + 12);
    const tones = [root, third, fifth, root + 12];
    const notes: ScoreNote[] = [];
    for (let step = 0; step * stepBeats < BEATS_PER_BAR; step += 1) {
      const offset = step * stepBeats;
      notes.push({
        beat: bar.start + offset,
        midi: tones[pattern[step % pattern.length]],
        duration: stepBeats,
        velocity: Number.isInteger(offset) ? velocity + 0.15 : velocity,
      });
    }
    return notes;
  };

  return bars.flatMap((bar) => {
    switch (bar.section) {
      case 'intro':
        return run(bar, 0.5, 0.55);
      case 'dialogue':
        return dialogueTurn('arpeggio', bar);
      case 'build':
        return bar.barInSection < 4
          ? run(bar, 0.5, 0.5)
          : run(bar, 0.25, 0.45 + 0.35 * progress(bar, 8));
      case 'coda':
        if (bar.barInSection === 0) {
          return figureNotes(
            bar.start,
            [
              [0, 86, 1],
              [1, 81, 1],
              [2, 77, 1],
              [3, 74, 1],
            ],
            0.55
          );
        }
        // C# rising to the D that opens the intro.
        return bar.barInSection === 3
          ? figureNotes(
              bar.start,
              [69, 73, 76, 81, 85, 81, 76, 73].map(
                (midi, step): Figure => [step / 2, midi, 0.5]
              ),
              0.45
            )
          : [];
      default:
        return run(bar, 0.5, 0.5);
    }
  });
}

function bassPart(bars: Bar[]): ScoreNote[] {
  const groove = (bar: Bar, velocity: number) => {
    const root = bar.harmony.bass;
    return figureNotes(
      bar.start,
      [
        [0, root, 1.25],
        [1.5, root, 0.5],
        [2, root + 12, 0.5],
        [2.5, root, 1],
        [3.5, root + 7, 0.5],
      ],
      velocity
    );
  };

  return bars.flatMap((bar) => {
    const root = bar.harmony.bass;
    switch (bar.section) {
      case 'theme':
        return groove(bar, 0.75);
      case 'dialogue':
        return dialogueTurn('bass', bar);
      case 'build':
        if (bar.barInSection < 4) {
          return figureNotes(
            bar.start,
            [
              [0, root, 2],
              [2, root, 2],
            ],
            0.7
          );
        }
        return Array.from({ length: 8 }, (_, step) => ({
          beat: bar.start + step / 2,
          midi: step % 2 === 0 ? root : root + 12,
          duration: 0.5,
          velocity: 0.6 + 0.35 * progress(bar, 8),
        }));
      case 'reprise':
        return groove(bar, 0.85);
      case 'coda':
        if (bar.barInSection === 0) {
          return figureNotes(bar.start, [[0, root, 4]], 0.85);
        }
        return bar.barInSection === 3
          ? figureNotes(
              bar.start,
              [
                [0, root, 2],
                [2, root + 7, 1],
                [3, root + 4, 1],
              ],
              0.6
            )
          : [];
      default:
        return [];
    }
  });
}

// General MIDI percussion keys, played on the recorded kit (drumKit.ts).
const KICK = 36;
const SNARE = 38;
const CLAP = 39;
const CLOSED_HAT = 42;
const OPEN_HAT = 46;
const CRASH = 49;

function percussionPart(bars: Bar[]): ScoreNote[] {
  const hats = (bar: Bar, stepBeats: number, velocity: number) => {
    const notes: ScoreNote[] = [];
    for (let offset = 0; offset < BEATS_PER_BAR; offset += stepBeats) {
      notes.push({
        beat: bar.start + offset,
        midi: CLOSED_HAT,
        duration: 0.1,
        velocity: Number.isInteger(offset) ? velocity + 0.15 : velocity,
      });
    }
    return notes;
  };
  const hits =
    (midi: number) => (bar: Bar, offsets: number[], velocity: number) =>
      offsets.map((offset) => ({
        beat: bar.start + offset,
        midi,
        duration: 0.25,
        velocity,
      }));
  const claps = hits(CLAP);
  const snares = hits(SNARE);
  const kicks = hits(KICK);
  // An open hat on the last off-beat of every other bar.
  const backbeat = (bar: Bar, velocity: number) => {
    const openHatBeat =
      bar.barInSection % 2 === 1 ? bar.start + 3.5 : Number.NaN;
    const hatLine = hats(bar, 0.5, velocity - 0.2).map((note) =>
      note.beat === openHatBeat
        ? { ...note, midi: OPEN_HAT, duration: 0.5 }
        : note
    );
    return [
      ...hatLine,
      ...kicks(bar, [0, 2], velocity),
      ...snares(bar, [1, 3], velocity),
    ];
  };

  return bars.flatMap((bar) => {
    switch (bar.section) {
      case 'theme':
        return backbeat(bar, 0.75);
      case 'dialogue':
        return bar.barInSection === 7
          ? [...hats(bar, 1, 0.25), ...claps(bar, [0], 1)]
          : hats(bar, 1, 0.2);
      case 'build': {
        const swell = progress(bar, 8);
        if (bar.barInSection < 4) {
          return backbeat(bar, 0.65);
        }
        if (bar.barInSection < 6) {
          return [
            ...hats(bar, 0.25, 0.3 + 0.3 * swell),
            ...kicks(bar, [0, 2], 0.8),
            ...snares(bar, [1, 3], 0.8),
          ];
        }
        if (bar.barInSection === 6) {
          return [
            ...hats(bar, 0.25, 0.55),
            ...kicks(bar, [0, 1, 2, 3], 0.85),
            ...claps(bar, [1, 3], 0.85),
          ];
        }
        // A snare roll swelling into the reprise.
        return snares(
          bar,
          [0, 0.5, 1, 1.5, 2, 2.5, 3, 3.25, 3.5, 3.75],
          0.9
        ).map((note, index, roll) => ({
          ...note,
          velocity: 0.55 + (0.45 * index) / (roll.length - 1),
        }));
      }
      case 'reprise':
        return backbeat(bar, 0.85);
      case 'coda':
        if (bar.barInSection === 0) {
          return [
            ...kicks(bar, [0], 1),
            { beat: bar.start, midi: CRASH, duration: 2, velocity: 0.8 },
          ];
        }
        return bar.barInSection === 3 ? hats(bar, 0.5, 0.2).slice(4) : [];
      default:
        return [];
    }
  });
}

function relayScore(): Score {
  const bars = relayBars();
  const parts: ScorePart[] = [
    {
      name: 'Lead',
      color: '#FF5E7A',
      timbre: 'lead',
      instrument: 'violin',
      notes: leadPart(bars),
    },
    {
      name: 'Pad',
      color: '#9B7BFF',
      timbre: 'pad',
      instrument: 'string_ensemble_1',
      notes: padPart(bars),
    },
    {
      name: 'Arpeggio',
      color: '#38ACDD',
      timbre: 'pluck',
      instrument: 'orchestral_harp',
      notes: arpeggioPart(bars),
    },
    {
      name: 'Bass',
      color: '#FF9F43',
      timbre: 'pluck',
      instrument: 'pizzicato_strings',
      notes: bassPart(bars),
    },
    {
      name: 'Counter-melody',
      color: '#FFD61E',
      timbre: 'bell',
      instrument: 'flute',
      notes: counterPart(bars),
    },
    {
      name: 'Percussion',
      color: '#8BE38B',
      timbre: 'kit',
      instrument: DRUMS,
      notes: percussionPart(bars),
    },
  ];

  return {
    id: 'relay',
    title: 'Relay in D minor',
    bpm: 100,
    lengthBeats: bars.length * BEATS_PER_BAR,
    parts,
  };
}

const here = dirname(fileURLToPath(import.meta.url));

export const sampleStore = new SampleStore(join(here, 'samples'));

// Imported parts get the instrument their General MIDI program names.
const instrumentNames = await sampleStore.instrumentNames();

export const scores: Record<string, Score> = {
  relay: relayScore(),
  // Every .mid file in ./midi (see README).
  ...loadMidiFolder(join(here, 'midi'), instrumentNames),
};

export const DEFAULT_SCORE_ID = 'relay';
