// Recorded instrument notes, fetched from the orchestra server's /samples
// mirror and decoded once: FluidR3_GM records every semitone, so a note plays
// its own sample at its natural speed; the drum kit has a few takes per piece,
// picked at random so repeated hits differ.
import type { AudioBuffer, BaseAudioContext } from 'react-native-audio-api';

import { DRUM_PIECES, DRUMS, drumPieceFor } from '../../../orchestra-server/drumKit';

const LOWEST_SAMPLED_MIDI = 21;
const HIGHEST_SAMPLED_MIDI = 108;
/** Decodes in flight at once: enough to fill a part quickly on a LAN. */
const CONCURRENT_LOADS = 4;
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
    }));
  }
  const note = sampledMidi(request.midi);
  const name = `${NOTE_NAMES[note % 12]}${Math.floor(note / 12) - 1}`;
  return [
    { sound, url: `${baseUrl}/samples/${request.instrument}/${name}.mp3` },
  ];
}

export default class InstrumentBank {
  private readonly context: BaseAudioContext;
  private readonly baseUrl: string | null;
  private readonly sounds = new Map<string, AudioBuffer[]>();
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
  get(instrument: string, midi: number): AudioBuffer | null {
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
        this.sounds.set(take.sound, [
          ...(this.sounds.get(take.sound) ?? []),
          buffer,
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
