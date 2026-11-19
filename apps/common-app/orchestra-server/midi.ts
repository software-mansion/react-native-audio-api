// Standard MIDI File (SMF) import: turns a .mid file into an orchestra score.
// Dependency-free. Supports format 0 and 1 with ticks-per-quarter timing.
//
// Each (track, channel) pair with notes becomes one part. Channel 10 (drums)
// becomes a 'kit' part on the recorded drum kit. Parts too low for phone speakers move up whole
// octaves. An optional JSON config next to the file overrides the title,
// tempo and per-part timbre / phone cap / transposition, and can split a drum
// part into one part per group of kit pieces, so several phones share the kit.
import { existsSync, readdirSync, readFileSync } from 'node:fs';
import { basename, extname, join } from 'node:path';

import { DRUM_PIECES, DRUMS, drumPieceFor, type DrumPiece } from './drumKit.ts';

import type {
  Score,
  ScoreNote,
  ScorePart,
  Timbre,
} from '../src/demos/Orchestra/protocol.ts';

export interface MidiNote {
  startTick: number;
  endTick: number;
  key: number;
  velocity: number;
}

export interface MidiTrack {
  name: string | null;
  /** Notes per MIDI channel (0-15). */
  notesByChannel: Map<number, MidiNote[]>;
  /** The first General MIDI program each channel selects in this track. */
  programByChannel: Map<number, number>;
}

export interface MidiFile {
  format: number;
  ticksPerQuarter: number;
  /** Microseconds per quarter note from the first Set Tempo event, if any. */
  tempo: number | null;
  /** Numerator of the first Time Signature event, if any. */
  beatsPerBar: number | null;
  /** Number of Set Tempo events; only the first is honoured. */
  tempoChanges: number;
  /**
   * The first program per channel anywhere in the file, for tracks that set
   * none.
   */
  programByChannel: Map<number, number>;
  tracks: MidiTrack[];
}

class ByteReader {
  private readonly bytes: Uint8Array;
  position: number;
  readonly end: number;

  constructor(bytes: Uint8Array, start = 0, end = bytes.length) {
    this.bytes = bytes;
    this.position = start;
    this.end = end;
  }

  get done(): boolean {
    return this.position >= this.end;
  }

  u8(): number {
    if (this.position >= this.end) {
      throw new Error('Unexpected end of MIDI data');
    }
    return this.bytes[this.position++];
  }

  u16(): number {
    return (this.u8() << 8) | this.u8();
  }

  u32(): number {
    return (
      ((this.u8() << 24) >>> 0) +
      (this.u8() << 16) +
      (this.u8() << 8) +
      this.u8()
    );
  }

  ascii(length: number): string {
    let text = '';
    for (let i = 0; i < length; i += 1) {
      text += String.fromCharCode(this.u8());
    }
    return text;
  }

  /** Variable-length quantity: 7 bits per byte, high bit means "more". */
  vlq(): number {
    let value = 0;
    for (let i = 0; i < 4; i += 1) {
      const byte = this.u8();
      value = (value << 7) | (byte & 0x7f);
      if ((byte & 0x80) === 0) {
        return value;
      }
    }
    throw new Error('Variable-length quantity longer than 4 bytes');
  }

  skip(length: number): void {
    if (this.position + length > this.end) {
      throw new Error('Unexpected end of MIDI data');
    }
    this.position += length;
  }
}

export function parseMidi(bytes: Uint8Array): MidiFile {
  const reader = new ByteReader(bytes);
  if (reader.ascii(4) !== 'MThd') {
    throw new Error('Not a MIDI file (missing MThd header)');
  }
  const headerLength = reader.u32();
  const headerEnd = reader.position + headerLength;
  const format = reader.u16();
  const trackCount = reader.u16();
  const division = reader.u16();
  reader.position = headerEnd;

  if (format > 1) {
    throw new Error(`MIDI format ${format} is not supported (only 0 and 1)`);
  }
  if (division & 0x8000) {
    throw new Error('SMPTE time division is not supported');
  }

  const midi: MidiFile = {
    format,
    ticksPerQuarter: division,
    tempo: null,
    beatsPerBar: null,
    tempoChanges: 0,
    programByChannel: new Map(),
    tracks: [],
  };

  for (let index = 0; index < trackCount && !reader.done; index += 1) {
    const chunkType = reader.ascii(4);
    const chunkLength = reader.u32();
    const chunkEnd = reader.position + chunkLength;
    if (chunkType === 'MTrk') {
      midi.tracks.push(
        parseTrack(new ByteReader(bytes, reader.position, chunkEnd), midi)
      );
    } else {
      index -= 1; // Unknown chunks do not count as tracks.
    }
    reader.position = chunkEnd;
  }

  return midi;
}

function parseTrack(reader: ByteReader, midi: MidiFile): MidiTrack {
  const track: MidiTrack = {
    name: null,
    notesByChannel: new Map(),
    programByChannel: new Map(),
  };
  // Sounding notes per channel+key; a FIFO so overlapping repeats pair up in order.
  const open = new Map<number, { startTick: number; velocity: number }[]>();
  let tick = 0;
  let runningStatus = 0;

  const noteOff = (channel: number, key: number) => {
    const started = open.get(channel * 128 + key)?.shift();
    if (!started) {
      return;
    }
    const notes = track.notesByChannel.get(channel) ?? [];
    notes.push({
      startTick: started.startTick,
      endTick: tick,
      key,
      velocity: started.velocity,
    });
    track.notesByChannel.set(channel, notes);
  };

  while (!reader.done) {
    tick += reader.vlq();
    let status = reader.u8();

    if (status === 0xff) {
      const type = reader.u8();
      const length = reader.vlq();
      if (type === 0x2f) {
        break; // End of track.
      } else if (type === 0x03 && track.name === null) {
        track.name = reader.ascii(length).trim() || null;
      } else if (type === 0x51 && length === 3) {
        const tempo = (reader.u8() << 16) | (reader.u8() << 8) | reader.u8();
        midi.tempoChanges += 1;
        midi.tempo ??= tempo;
      } else if (type === 0x58 && length >= 1) {
        const numerator = reader.u8();
        midi.beatsPerBar ??= numerator;
        reader.skip(length - 1);
      } else {
        reader.skip(length);
      }
      continue;
    }

    if (status === 0xf0 || status === 0xf7) {
      reader.skip(reader.vlq()); // SysEx.
      continue;
    }

    if (status < 0x80) {
      // Running status: this byte is already the first data byte.
      if (runningStatus === 0) {
        throw new Error('Running status used before any channel message');
      }
      reader.position -= 1;
      status = runningStatus;
    } else {
      runningStatus = status;
    }

    const kind = status & 0xf0;
    const channel = status & 0x0f;
    if (kind === 0x90 || kind === 0x80) {
      const key = reader.u8();
      const velocity = reader.u8();
      if (kind === 0x90 && velocity > 0) {
        const slot = channel * 128 + key;
        const sounding = open.get(slot) ?? [];
        sounding.push({ startTick: tick, velocity });
        open.set(slot, sounding);
      } else {
        noteOff(channel, key);
      }
    } else if (kind === 0xc0) {
      const program = reader.u8();
      if (!track.programByChannel.has(channel)) {
        track.programByChannel.set(channel, program);
      }
      if (!midi.programByChannel.has(channel)) {
        midi.programByChannel.set(channel, program);
      }
    } else if (kind === 0xd0) {
      reader.skip(1);
    } else {
      reader.skip(2); // Aftertouch, control change, pitch bend.
    }
  }

  // Notes never released end with the track.
  for (const [slot, sounding] of open) {
    for (let i = sounding.length; i > 0; i -= 1) {
      noteOff(Math.floor(slot / 128), slot % 128);
    }
  }
  return track;
}

// ---- MIDI → orchestra score -------------------------------------------------

export interface MidiPartConfig {
  timbre?: Timbre;
  /** A FluidR3_GM instrument name; replaces the one the file's program selects. */
  instrument?: string;
  maxPlayers?: number;
  /** Semitones; replaces the automatic octave shift. */
  transpose?: number;
  skip?: boolean;
  color?: string;
  /**
   * Drum parts only: replaces the part with one part per entry, named `<part>:
   * <group>` and holding the hits of the listed kit pieces. Pieces no group
   * lists stay in a part with the original name. The new parts take their own
   * entries in `parts`.
   */
  splitDrums?: Record<string, DrumPiece[]>;
}

export interface MidiScoreConfig {
  title?: string;
  /** Overrides the file's tempo. */
  bpm?: number;
  /** Keyed by the part names the server prints at startup. */
  parts?: Record<string, MidiPartConfig>;
}

export const TIMBRES: readonly Timbre[] = [
  'bell',
  'pluck',
  'pad',
  'lead',
  'hat',
  'clap',
  'kit',
  'click',
];

const DRUM_CHANNEL = 9;
const LOWEST_PHONE_MIDI = 55;
const HIGHEST_PHONE_MIDI = 96;
const PART_COLORS = [
  '#38ACDD',
  '#FFD61E',
  '#FF7774',
  '#8BE38B',
  '#7B5CFF',
  '#FF9F1C',
  '#F4F1DE',
  '#E8443A',
];

export interface ImportedPart {
  name: string;
  noteCount: number;
  lowest: number;
  highest: number;
  transpose: number;
  timbre: Timbre;
  instrument?: string;
  maxPlayers?: number;
  skipped: boolean;
}

export interface MidiImport {
  score: Score;
  parts: ImportedPart[];
  warnings: string[];
}

/**
 * `instrumentNames` lists the FluidR3_GM instruments by General MIDI program;
 * when it is empty, parts get no recorded instrument and phones use the synth.
 */
export function midiToScore(
  id: string,
  midi: MidiFile,
  config: MidiScoreConfig = {},
  instrumentNames: readonly string[] = []
): MidiImport {
  const warnings: string[] = [];
  const quarterTicks = midi.ticksPerQuarter;
  const fileBpm = midi.tempo ? 60_000_000 / midi.tempo : 120;
  if (midi.tempoChanges > 1) {
    warnings.push(
      `${midi.tempoChanges} tempo changes; only the first (${fileBpm.toFixed(1)} bpm) is used`
    );
  }

  const rawParts = inEntranceOrder(
    splitDrumParts(collectRawParts(midi), config, warnings)
  );
  const knownNames = new Set(rawParts.map((part) => part.name));
  for (const [name, partConfig] of Object.entries(config.parts ?? {})) {
    if (!knownNames.has(name) && !partConfig.splitDrums) {
      warnings.push(
        `config names part "${name}", which the file does not have`
      );
    }
  }

  const parts: ScorePart[] = [];
  const imported: ImportedPart[] = [];
  let lastBeat = 0;

  rawParts.forEach((raw, index) => {
    const partConfig = config.parts?.[raw.name] ?? {};
    const isDrums = raw.channel === DRUM_CHANNEL;
    const timbre =
      validTimbre(partConfig.timbre, warnings, raw.name) ??
      (isDrums ? 'kit' : 'pluck');
    const instrument =
      partConfig.instrument ?? (isDrums ? DRUMS : instrumentNames[raw.program]);

    const keys = raw.notes.map((note) => note.key);
    const lowest = Math.min(...keys);
    const highest = Math.max(...keys);
    const transpose = isDrums
      ? 0
      : (partConfig.transpose ?? phoneTranspose(lowest, highest));

    const notes: ScoreNote[] = [];
    for (const note of raw.notes) {
      const midiKey = isDrums ? drumKey(note.key) : note.key + transpose;
      if (midiKey === null) {
        continue;
      }
      const beat = note.startTick / quarterTicks;
      const duration = Math.max(
        0.05,
        (note.endTick - note.startTick) / quarterTicks
      );
      lastBeat = Math.max(lastBeat, beat + duration);
      notes.push({
        beat,
        midi: midiKey,
        duration,
        velocity: note.velocity / 127,
      });
    }

    imported.push({
      name: raw.name,
      noteCount: notes.length,
      lowest,
      highest,
      transpose,
      timbre,
      instrument,
      maxPlayers: partConfig.maxPlayers,
      skipped: Boolean(partConfig.skip) || notes.length === 0,
    });
    if (partConfig.skip || notes.length === 0) {
      return;
    }

    parts.push({
      name: raw.name,
      color: partConfig.color ?? PART_COLORS[index % PART_COLORS.length],
      timbre,
      ...(instrument ? { instrument } : {}),
      notes,
      ...(partConfig.maxPlayers ? { maxPlayers: partConfig.maxPlayers } : {}),
    });
  });

  if (parts.length === 0) {
    throw new Error('no playable notes');
  }

  const beatsPerBar = midi.beatsPerBar ?? 4;
  return {
    score: {
      id,
      title: config.title ?? id,
      bpm: Math.round((config.bpm ?? fileBpm) * 10) / 10,
      lengthBeats: Math.ceil(lastBeat / beatsPerBar) * beatsPerBar,
      beatsPerBar,
      parts,
    },
    parts: imported,
    warnings,
  };
}

interface RawPart {
  name: string;
  channel: number;
  /** General MIDI program, 0 (piano) when the file never sets one. */
  program: number;
  notes: MidiNote[];
}

function collectRawParts(midi: MidiFile): RawPart[] {
  const parts: RawPart[] = [];
  midi.tracks.forEach((track, trackIndex) => {
    const channels = [...track.notesByChannel.keys()].sort((a, b) => a - b);
    for (const channel of channels) {
      const base = track.name ?? `Track ${trackIndex + 1}`;
      const name =
        channels.length > 1 || channel === DRUM_CHANNEL
          ? `${base} (ch ${channel + 1})`
          : base;
      const notes = [...track.notesByChannel.get(channel)!].sort(
        (a, b) => a.startTick - b.startTick
      );
      const program =
        track.programByChannel.get(channel) ??
        midi.programByChannel.get(channel) ??
        0;
      parts.push({ name: uniqueName(name, parts), channel, program, notes });
    }
  });
  return parts;
}

// Phones are dealt parts in order, so the parts that open the song come first;
// among parts entering together, the busier one wins.
function inEntranceOrder(parts: RawPart[]): RawPart[] {
  return parts.sort(
    (a, b) =>
      a.notes[0].startTick - b.notes[0].startTick ||
      b.notes.length - a.notes.length
  );
}

function splitDrumParts(
  rawParts: RawPart[],
  config: MidiScoreConfig,
  warnings: string[]
): RawPart[] {
  return rawParts.flatMap((raw) => {
    const groups = config.parts?.[raw.name]?.splitDrums;
    if (!groups) {
      return [raw];
    }
    if (raw.channel !== DRUM_CHANNEL) {
      warnings.push(`part "${raw.name}": splitDrums ignored, not a drum part`);
      return [raw];
    }

    const groupByPiece = new Map<DrumPiece, string>();
    for (const [group, pieces] of Object.entries(groups)) {
      for (const piece of pieces) {
        if (!(piece in DRUM_PIECES)) {
          warnings.push(
            `part "${raw.name}": unknown drum piece "${piece}" (use ${Object.keys(DRUM_PIECES).join(', ')})`
          );
        } else {
          groupByPiece.set(piece, group);
        }
      }
    }

    const notesByName = new Map<string, MidiNote[]>(
      [...Object.keys(groups), null].map((group) => [
        group === null ? raw.name : `${raw.name}: ${group}`,
        [],
      ])
    );
    for (const note of raw.notes) {
      const piece = drumPieceFor(note.key);
      const group = piece ? groupByPiece.get(piece) : undefined;
      notesByName
        .get(group === undefined ? raw.name : `${raw.name}: ${group}`)!
        .push(note);
    }
    return [...notesByName]
      .filter(([, notes]) => notes.length > 0)
      .map(([name, notes]) => ({ ...raw, name, notes }));
  });
}

function uniqueName(name: string, existing: RawPart[]): string {
  let candidate = name;
  for (let n = 2; existing.some((part) => part.name === candidate); n += 1) {
    candidate = `${name} #${n}`;
  }
  return candidate;
}

/**
 * Whole octaves up until the part clears the phone floor, without passing the
 * ceiling.
 */
function phoneTranspose(lowest: number, highest: number): number {
  let shift = 0;
  while (
    lowest + shift < LOWEST_PHONE_MIDI &&
    highest + shift + 12 <= HIGHEST_PHONE_MIDI
  ) {
    shift += 12;
  }
  return shift;
}

// Drum keys stay General MIDI keys; ones the kit has no piece for are dropped.
function drumKey(key: number): number | null {
  return drumPieceFor(key) ? key : null;
}

function validTimbre(
  timbre: string | undefined,
  warnings: string[],
  partName: string
): Timbre | undefined {
  if (timbre === undefined) {
    return undefined;
  }
  if ((TIMBRES as readonly string[]).includes(timbre)) {
    return timbre as Timbre;
  }
  warnings.push(
    `part "${partName}": unknown timbre "${timbre}" (use ${TIMBRES.join(', ')})`
  );
  return undefined;
}

// ---- Loading a folder of .mid files -----------------------------------------

/**
 * Imports every `<name>.mid` in `directory` as score `midi-<name>`, with
 * `<name>.json` as its optional config. A broken file is reported and skipped.
 */
export function loadMidiFolder(
  directory: string,
  instrumentNames: readonly string[] = []
): Record<string, Score> {
  if (!existsSync(directory)) {
    return {};
  }

  const scores: Record<string, Score> = {};
  for (const file of readdirSync(directory).sort()) {
    const extension = extname(file).toLowerCase();
    if (extension !== '.mid' && extension !== '.midi') {
      continue;
    }
    const name = basename(file, extname(file));
    const slug = name
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, '-')
      .replace(/^-|-$/g, '');
    const id = `midi-${slug}`;
    try {
      const configPath = join(directory, `${name}.json`);
      const config = existsSync(configPath)
        ? (JSON.parse(readFileSync(configPath, 'utf8')) as MidiScoreConfig)
        : {};
      const midi = parseMidi(
        new Uint8Array(readFileSync(join(directory, file)))
      );
      const result = midiToScore(
        id,
        midi,
        { title: name, ...config },
        instrumentNames
      );
      scores[id] = result.score;
      printImport(file, result);
    } catch (error) {
      console.warn(`MIDI ${file}: skipped — ${(error as Error).message}`);
    }
  }
  return scores;
}

function printImport(
  file: string,
  { score, parts, warnings }: MidiImport
): void {
  console.log(
    `MIDI ${file} → "${score.title}" (${score.id}), ${score.bpm} bpm, ${score.lengthBeats} beats`
  );
  for (const part of parts) {
    const details = [
      `${part.noteCount} notes`,
      `midi ${part.lowest}-${part.highest}`,
      part.transpose
        ? `transposed ${part.transpose > 0 ? '+' : ''}${part.transpose}`
        : null,
      part.instrument ?? part.timbre,
      part.maxPlayers ? `max ${part.maxPlayers} phone(s)` : null,
      part.skipped ? 'SKIPPED' : null,
    ].filter(Boolean);
    console.log(`  part "${part.name}": ${details.join(', ')}`);
  }
  for (const warning of warnings) {
    console.warn(`  warning: ${warning}`);
  }
}
