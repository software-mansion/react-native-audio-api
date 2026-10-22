// Lookahead scheduler: every tick it hands the synth the notes of this phone's
// parts (merged, each keeping its part's voice) that sound within the next
// LOOKAHEAD_MS, reshaped by the console's note controls, converting the server-time
// beat grid → local performance time (ClockSync) → the context time at which a
// sound is *heard* then (AudioClockMap, output latency included). Scheduling
// only a short window ahead keeps late clock corrections cheap: nothing already
// queued is older than one lookahead. The window must outlast one hardware
// buffer plus output latency, which reaches ~150 ms on some Android devices.
import type { AudioContext } from 'react-native-audio-api';

import type ClockSync from './ClockSync';
import type AudioClockMap from './AudioClockMap';
import type Synth from './Synth';
import {
  INSTRUMENT_CHOICES,
  isPercussion,
  STUTTER_STEP_BEATS,
  type ConsoleSettings,
} from './Console';
import { DRUMS } from '../../../orchestra-server/drumKit';
import type { InstrumentRequest } from './InstrumentBank';
import type { SampleRequest } from './SampleBank';
import { resolveTimbre } from './Voices';
import type { Score, ScoreNote, Timbre, Transport } from './protocol';

const TICK_MS = 50;
const LOOKAHEAD_MS = 400;
/** Notes this late are dropped instead of played in a clump. */
const MAX_LATE_MS = 30;
/** Stutter repeats sound for this fraction of their grid step. */
const STUTTER_GATE = 0.55;
const LOWEST_MIDI = 24;
const HIGHEST_MIDI = 108;

export interface SchedulerConfig {
  transport: Transport;
  score: Score;
  partIndices: readonly number[];
  /** Only the note controls are read here; the synth applies the rest. */
  console: ConsoleSettings;
  /** Keep the cursor and beat flashes going, but schedule no sound. */
  muted: boolean;
  /**
   * Manual correction on top of the measured output latency; positive plays
   * earlier.
   */
  trimMs: number;
}

/** `beat` counts from the transport start, across loops of the score. */
export type BeatListener = (beat: number) => void;

/** A note as it is heard, cued at the moment it sounds. */
export interface NoteCue {
  /** Unique per playing of a note: `${cycle}:${index into mergeParts()}`. */
  key: string;
  midi: number;
  timbre: Timbre;
  partIndex: number;
  durationMs: number;
}

export type NoteListener = (cue: NoteCue) => void;

export interface PartNote extends ScoreNote {
  timbre: Timbre;
  instrument?: string;
  partIndex: number;
}

/**
 * This phone's parts as one list, sorted by beat (stable, so ties keep part
 * order).
 */
export function mergeParts(
  score: Score,
  partIndices: readonly number[]
): PartNote[] {
  return partIndices
    .flatMap((partIndex) => {
      const part = score.parts[partIndex];
      return part
        ? part.notes.map((note) => ({
            ...note,
            timbre: part.timbre,
            instrument: part.instrument,
            partIndex,
          }))
        : [];
    })
    .sort((a, b) => a.beat - b.beat);
}

interface Cursor {
  epoch: number;
  cycle: number;
  noteIndex: number;
  beat: number;
}

export default class Scheduler {
  private readonly context: AudioContext;
  private readonly clockSync: ClockSync;
  private readonly clockMap: AudioClockMap;
  private readonly synth: Synth;

  private config: SchedulerConfig | null = null;
  private notes: PartNote[] = [];
  private partsKey = '';
  private samplesKey = '';
  private cursor: Cursor | null = null;
  private timer: ReturnType<typeof setInterval> | null = null;
  private cueTimers = new Set<ReturnType<typeof setTimeout>>();
  private beatListener: BeatListener | null = null;
  private noteListener: NoteListener | null = null;

  constructor(
    context: AudioContext,
    clockSync: ClockSync,
    clockMap: AudioClockMap,
    synth: Synth
  ) {
    this.context = context;
    this.clockSync = clockSync;
    this.clockMap = clockMap;
    this.synth = synth;
  }

  onBeat(listener: BeatListener | null): void {
    this.beatListener = listener;
  }

  onNote(listener: NoteListener | null): void {
    this.noteListener = listener;
  }

  configure(config: SchedulerConfig): void {
    const previous = this.config;
    this.config = config;

    const partsKey = `${config.score.id}:${config.partIndices.join(',')}`;
    const partChanged = partsKey !== this.partsKey;
    if (partChanged) {
      this.partsKey = partsKey;
      this.notes = mergeParts(config.score, config.partIndices);
    }
    this.prepareSamples(config);
    if (partChanged || previous?.transport.epoch !== config.transport.epoch) {
      this.cursor = null;
    }
    if (!config.transport.playing) {
      this.clearCueTimers();
    }
  }

  start(): void {
    if (!this.timer) {
      this.timer = setInterval(() => this.tick(), TICK_MS);
    }
  }

  stop(): void {
    if (this.timer) {
      clearInterval(this.timer);
      this.timer = null;
    }
    this.clearCueTimers();
    this.cursor = null;
  }

  // Loads every recorded note and bakes every synth voice these parts can ask
  // for before their notes come up, so the first time round does not fall
  // back to the costlier live voices.
  private prepareSamples(config: SchedulerConfig): void {
    const { voice, octave } = config.console;
    const key = `${this.partsKey}:${voice}:${octave}`;
    if (key === this.samplesKey) {
      return;
    }
    this.samplesKey = key;

    const synthRequests = new Map<string, SampleRequest>();
    const recordedRequests = new Map<string, InstrumentRequest>();
    for (const note of this.notes) {
      const { timbre, instrument, midi } = voiceFor(note, config.console);
      if (instrument) {
        recordedRequests.set(`${instrument}:${midi}`, { instrument, midi });
      } else {
        const resolved = resolveTimbre(timbre, midi);
        synthRequests.set(`${resolved}:${midi}`, { timbre: resolved, midi });
      }
    }
    this.synth.prepare([...synthRequests.values()]);
    this.synth.prepareInstruments([...recordedRequests.values()]);
  }

  private tick(): void {
    const config = this.config;
    if (!config?.transport.playing || !this.clockSync.isSynced) {
      return;
    }

    const now = performance.now();
    const horizon = now + LOOKAHEAD_MS;
    const msPerBeat = 60000 / config.transport.bpm;
    const cursor = this.cursor ?? this.cursorAt(now, config, msPerBeat);
    this.cursor = cursor;

    for (;;) {
      const noteSoundsAt = this.soundTime(
        this.cursorBeat(cursor, config),
        config,
        msPerBeat
      );
      if (noteSoundsAt > horizon) {
        break;
      }

      if (noteSoundsAt >= now - MAX_LATE_MS) {
        this.scheduleAt(cursor, noteSoundsAt, config, msPerBeat);
      }
      this.advance(cursor, config);
    }
  }

  // Which note is next when playback starts or the cursor was reset: skip
  // everything that already sounded.
  private cursorAt(
    now: number,
    config: SchedulerConfig,
    msPerBeat: number
  ): Cursor {
    const elapsedMs =
      this.clockSync.localToServer(now) - config.transport.startAt;
    const elapsedBeats = Math.max(0, elapsedMs / msPerBeat);
    const cycle = Math.floor(elapsedBeats / config.score.lengthBeats);
    const beatInCycle = elapsedBeats - cycle * config.score.lengthBeats;

    const cursor: Cursor = {
      epoch: config.transport.epoch,
      cycle,
      noteIndex: 0,
      beat: Math.ceil(beatInCycle),
    };
    while (
      cursor.noteIndex < this.notes.length &&
      this.notes[cursor.noteIndex].beat < beatInCycle
    ) {
      cursor.noteIndex += 1;
    }
    return cursor;
  }

  // The cursor walks notes and whole beats together; whichever comes first is
  // the next thing to schedule.
  private cursorBeat(cursor: Cursor, config: SchedulerConfig): number {
    const note = this.notes[cursor.noteIndex];
    const noteBeat = note ? note.beat : Number.POSITIVE_INFINITY;
    const nextBeat =
      cursor.beat < config.score.lengthBeats
        ? cursor.beat
        : Number.POSITIVE_INFINITY;
    return (
      cursor.cycle * config.score.lengthBeats + Math.min(noteBeat, nextBeat)
    );
  }

  private scheduleAt(
    cursor: Cursor,
    soundsAt: number,
    config: SchedulerConfig,
    msPerBeat: number
  ): void {
    const beatInCycle =
      this.cursorBeat(cursor, config) - cursor.cycle * config.score.lengthBeats;
    const note = this.notes[cursor.noteIndex];

    if (note && note.beat === beatInCycle && !config.muted) {
      this.playNote(note, cursor, config, msPerBeat);
    }

    if (note && note.beat === beatInCycle && !config.muted) {
      const cue: NoteCue = {
        key: `${cursor.cycle}:${cursor.noteIndex}`,
        midi: note.midi,
        timbre: note.timbre,
        partIndex: note.partIndex,
        durationMs: note.duration * msPerBeat,
      };
      this.cueAt(soundsAt, () => this.noteListener?.(cue));
    }

    if (cursor.beat === beatInCycle) {
      const beat = cursor.cycle * config.score.lengthBeats + beatInCycle;
      this.cueAt(soundsAt, () => this.beatListener?.(beat));
    }
  }

  // Runs `cue` when what was scheduled for `soundsAt` is heard.
  private cueAt(soundsAt: number, cue: () => void): void {
    const timer = setTimeout(
      () => {
        this.cueTimers.delete(timer);
        cue();
      },
      Math.max(0, soundsAt - performance.now())
    );
    this.cueTimers.add(timer);
  }

  private playNote(
    note: PartNote,
    cursor: Cursor,
    config: SchedulerConfig,
    msPerBeat: number
  ): void {
    const { console: controls } = config;
    const { timbre, instrument, midi } = voiceFor(note, controls);
    const lengthBeats = note.duration;
    const cycleStart = cursor.cycle * config.score.lengthBeats;

    // A stutter re-fires the note on the beat grid for as long as it lasts.
    const step = STUTTER_STEP_BEATS[controls.stutter];
    const hits = step
      ? Array.from(
          { length: Math.max(1, Math.round(lengthBeats / step)) },
          (_, i) => ({
            beat: note.beat + i * step,
            length: step * STUTTER_GATE,
          })
        )
      : [{ beat: note.beat, length: lengthBeats }];

    for (const hit of hits) {
      const soundsAt = this.soundTime(cycleStart + hit.beat, config, msPerBeat);
      const when = this.clockMap.startTimeHeardAt(soundsAt - config.trimMs);
      this.synth.play(
        Math.max(when, this.context.currentTime),
        midi,
        (hit.length * msPerBeat) / 1000,
        note.velocity,
        timbre,
        instrument
      );
    }
  }

  private advance(cursor: Cursor, config: SchedulerConfig): void {
    const beatInCycle =
      this.cursorBeat(cursor, config) - cursor.cycle * config.score.lengthBeats;
    const note = this.notes[cursor.noteIndex];

    if (note && note.beat === beatInCycle) {
      cursor.noteIndex += 1;
    }
    if (cursor.beat === beatInCycle) {
      cursor.beat += 1;
    }
    if (
      cursor.noteIndex >= this.notes.length &&
      cursor.beat >= config.score.lengthBeats
    ) {
      cursor.cycle += 1;
      cursor.noteIndex = 0;
      cursor.beat = 0;
    }
  }

  private soundTime(
    absoluteBeat: number,
    config: SchedulerConfig,
    msPerBeat: number
  ): number {
    return this.clockSync.serverToLocal(
      config.transport.startAt + absoluteBeat * msPerBeat
    );
  }

  private clearCueTimers(): void {
    for (const timer of this.cueTimers) {
      clearTimeout(timer);
    }
    this.cueTimers.clear();
  }
}

// The sound and pitch a note plays with the console's voice and octave
// applied; percussion keeps both and is always synthesized. `timbre` is also
// the fallback while a recorded instrument is still loading.
function voiceFor(
  note: PartNote,
  controls: ConsoleSettings
): { timbre: Timbre; instrument?: string; midi: number } {
  if (isPercussion(note.timbre)) {
    const onKit = note.instrument === DRUMS && controls.voice !== 'synth';
    return {
      timbre: note.timbre,
      instrument: onKit ? DRUMS : undefined,
      midi: note.midi,
    };
  }
  const { voice } = controls;
  const instrument =
    voice === 'score'
      ? note.instrument
      : voice === 'synth'
        ? undefined
        : INSTRUMENT_CHOICES[voice];
  return {
    timbre: note.timbre,
    instrument,
    midi: Math.min(
      HIGHEST_MIDI,
      Math.max(LOWEST_MIDI, note.midi + 12 * controls.octave)
    ),
  };
}
