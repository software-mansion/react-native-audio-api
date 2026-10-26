// Recorded instrument notes, fetched from the orchestra server's /samples
// mirror and decoded once: FluidR3_GM records every semitone, so a note plays
// its own sample at its natural speed; the drum kit has a few takes per piece,
// picked at random so repeated hits differ. The renders are cut off a few
// seconds in, still holding for sustained instruments, so a sound that holds
// gets a crossfaded loop over its last steady stretch for longer notes.
import type { AudioBuffer, BaseAudioContext } from 'react-native-audio-api';

import {
  DRUM_PIECES,
  DRUMS,
  drumPieceFor,
} from '../../../orchestra-server/drumKit';
import { crossfadeLoopFrames } from './SampleBank';

const LOWEST_SAMPLED_MIDI = 21;
const HIGHEST_SAMPLED_MIDI = 108;
/** Decodes in flight at once: enough to fill a part quickly on a LAN. */
const CONCURRENT_LOADS = 4;
/** The loop: the stretch a long note repeats, ending at the render's cut. */
const LOOP_LENGTH_SEC = 2;
/** Keeps the attack out of the loop. */
const LOOP_EARLIEST_START_SEC = 0.6;
const LOOP_CROSSFADE_SEC = 0.12;
/** Below this, measured against the loudest moment, the sound has died away. */
const HOLD_MIN_LEVEL_DB = -24;
/** Falling more than this across the loop means it is still decaying. */
const HOLD_MAX_DECLINE_DB = 3;
const LEVEL_WINDOW_SEC = 0.05;
// The renders spell every black key as a flat.
const NOTE_NAMES = [
  'C',
  'Db',
  'D',
  'Eb',
  'E',
  'F',
  'Gb',
  'G',
  'Ab',
  'A',
  'Bb',
  'B',
];

export interface InstrumentRequest {
  /** FluidR3_GM name, e.g. `violin`. */
  instrument: string;
  midi: number;
}

export function sampledMidi(midi: number): number {
  return Math.min(HIGHEST_SAMPLED_MIDI, Math.max(LOWEST_SAMPLED_MIDI, midi));
}

interface Take {
  sound: string;
  url: string;
  /** Drum hits ring out on their own and are never looped. */
  loopable: boolean;
}

export interface RecordedTake {
  buffer: AudioBuffer;
  /** Set when the sound holds: where a note longer than the render loops. */
  loop: { startSec: number; endSec: number } | null;
}

// What a note sounds with: one buffer list per pitch, or per kit piece.
function soundKey(instrument: string, midi: number): string | null {
  if (instrument !== DRUMS) {
    return `${instrument}:${sampledMidi(midi)}`;
  }
  const piece = drumPieceFor(midi);
  return piece ? `${DRUMS}:${piece}` : null;
}

function takesOf(baseUrl: string, request: InstrumentRequest): Take[] {
  const sound = soundKey(request.instrument, request.midi);
  if (!sound) {
    return [];
  }
  if (request.instrument === DRUMS) {
    const piece = drumPieceFor(request.midi)!;
    return Array.from({ length: DRUM_PIECES[piece].takes }, (_, index) => ({
      sound,
      url: `${baseUrl}/samples/${DRUMS}/${piece}/${index + 1}.wav`,
      loopable: false,
    }));
  }
  const note = sampledMidi(request.midi);
  const name = `${NOTE_NAMES[note % 12]}${Math.floor(note / 12) - 1}`;
  return [
    {
      sound,
      url: `${baseUrl}/samples/${request.instrument}/${name}.mp3`,
      loopable: true,
    },
  ];
}

export default class InstrumentBank {
  private readonly context: BaseAudioContext;
  private readonly baseUrl: string | null;
  private readonly sounds = new Map<string, RecordedTake[]>();
  /**
   * Requested once; a failed load is not retried, so the synth keeps playing
   * it.
   */
  private readonly requested = new Set<string>();
  private queue: Take[] = [];
  private loading = 0;
  private disposed = false;

  /** `baseUrl`: the orchestra server over http, or null to play synth only. */
  constructor(context: BaseAudioContext, baseUrl: string | null) {
    this.context = context;
    this.baseUrl = baseUrl;
  }

  /** A loaded take for this note, or null while none has loaded. */
  get(instrument: string, midi: number): RecordedTake | null {
    const sound = soundKey(instrument, midi);
    const takes = sound ? this.sounds.get(sound) : undefined;
    if (!takes || takes.length === 0) {
      return null;
    }
    return takes[Math.floor(Math.random() * takes.length)];
  }

  /** Fetches and decodes, in the background, every take not loaded yet. */
  prepare(requests: readonly InstrumentRequest[]): void {
    if (!this.baseUrl) {
      return;
    }
    for (const request of requests) {
      for (const take of takesOf(this.baseUrl, request)) {
        if (!this.requested.has(take.url)) {
          this.requested.add(take.url);
          this.queue.push(take);
        }
      }
    }
    while (this.loading < CONCURRENT_LOADS && this.queue.length > 0) {
      this.loadNext();
    }
  }

  dispose(): void {
    this.disposed = true;
    this.queue = [];
    this.sounds.clear();
  }

  private async loadNext(): Promise<void> {
    const take = this.queue.shift();
    if (!take) {
      return;
    }
    this.loading += 1;
    try {
      const buffer = await this.context.decodeAudioData(take.url);
      if (!this.disposed) {
        const recorded = take.loopable
          ? withSustainLoop(this.context, buffer)
          : { buffer, loop: null };
        this.sounds.set(take.sound, [
          ...(this.sounds.get(take.sound) ?? []),
          recorded,
        ]);
      }
    } catch (error) {
      console.warn(`Orchestra: could not load ${take.url}`, error);
    } finally {
      this.loading -= 1;
      if (!this.disposed && this.queue.length > 0) {
        this.loadNext();
      }
    }
  }
}

// A copy of `buffer` whose last steady stretch is crossfaded into a seamless
// loop, or the buffer as decoded when the sound decays instead of holding.
function withSustainLoop(
  context: BaseAudioContext,
  buffer: AudioBuffer
): RecordedTake {
  const { sampleRate, numberOfChannels, length } = buffer;
  const channels = Array.from({ length: numberOfChannels }, (_, channel) =>
    buffer.getChannelData(channel)
  );
  const fadeFrames = Math.round(LOOP_CROSSFADE_SEC * sampleRate);
  const loopEnd = lastAudibleFrame(channels) + 1;
  const loopStart = Math.max(
    Math.round(LOOP_EARLIEST_START_SEC * sampleRate),
    loopEnd - Math.round(LOOP_LENGTH_SEC * sampleRate)
  );
  if (
    loopEnd - loopStart < 2 * fadeFrames ||
    !holds(channels, loopStart, loopEnd, sampleRate)
  ) {
    return { buffer, loop: null };
  }

  const looped = context.createBuffer(numberOfChannels, length, sampleRate);
  channels.forEach((samples, channel) => {
    crossfadeLoopFrames(samples, loopStart, loopEnd, fadeFrames);
    looped.copyToChannel(samples, channel);
  });
  return {
    buffer: looped,
    loop: { startSec: loopStart / sampleRate, endSec: loopEnd / sampleRate },
  };
}

function lastAudibleFrame(channels: readonly Float32Array[]): number {
  for (let frame = channels[0].length - 1; frame > 0; frame -= 1) {
    if (channels.some((samples) => Math.abs(samples[frame]) > 1e-4)) {
      return frame;
    }
  }
  return 0;
}

// Holding: the loop stays near the sound's loudest level, and its second half
// is not clearly quieter than its first (a decaying piano note is both).
function holds(
  channels: readonly Float32Array[],
  loopStart: number,
  loopEnd: number,
  sampleRate: number
): boolean {
  const middle = Math.round((loopStart + loopEnd) / 2);
  const peak = loudestWindowRms(channels, 0, loopEnd, sampleRate);
  const firstHalf = rms(channels, loopStart, middle);
  const secondHalf = rms(channels, middle, loopEnd);
  if (peak === 0 || secondHalf === 0) {
    return false;
  }
  return (
    decibels(rms(channels, loopStart, loopEnd) / peak) >= HOLD_MIN_LEVEL_DB &&
    decibels(firstHalf / secondHalf) <= HOLD_MAX_DECLINE_DB
  );
}

function loudestWindowRms(
  channels: readonly Float32Array[],
  from: number,
  to: number,
  sampleRate: number
): number {
  const window = Math.max(1, Math.round(LEVEL_WINDOW_SEC * sampleRate));
  let loudest = 0;
  for (let start = from; start + window <= to; start += window) {
    loudest = Math.max(loudest, rms(channels, start, start + window));
  }
  return loudest;
}

function rms(
  channels: readonly Float32Array[],
  from: number,
  to: number
): number {
  let sum = 0;
  for (const samples of channels) {
    for (let frame = from; frame < to; frame += 1) {
      sum += samples[frame] * samples[frame];
    }
  }
  return Math.sqrt(sum / (channels.length * Math.max(1, to - from)));
}

const decibels = (ratio: number) => 20 * Math.log10(ratio);
