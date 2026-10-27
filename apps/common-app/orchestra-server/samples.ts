// Recorded instruments for the phones: the FluidR3_GM soundfont rendered to
// one mp3 per note by gleitz/midi-js-soundfonts, and a drum kit from VCSL
// (credits in the README). The server mirrors the notes the scores use into
// ./samples, so phones only need the local network, and fetches anything else
// on first request.
import { mkdir, readFile, writeFile } from 'node:fs/promises';
import { dirname, join } from 'node:path';

import {
  DRUM_PIECES,
  DRUMS,
  drumPieceFor,
  type DrumPiece,
} from './drumKit.ts';
import type { Score } from '../src/demos/Orchestra/protocol.ts';

const UPSTREAM = 'https://gleitz.github.io/midi-js-soundfonts/FluidR3_GM';
const VCSL = 'https://raw.githubusercontent.com/sgossner/VCSL/master';
const SKINS = 'Membranophones/Struck Membranophones';
const METALS = 'Idiophones/Struck Idiophones';

// One mid-loud velocity layer per piece; its round robins are the takes.
const DRUM_SOURCES: Record<DrumPiece, readonly string[]> = {
  kick: [
    `${SKINS}/Bass Drum 1/BDrumNew_hit_v5_rr1_Sum.wav`,
    `${SKINS}/Bass Drum 1/BDrumNew_hit_v5_rr2_Sum.wav`,
  ],
  snare: [
    `${SKINS}/Snare Drum, Modern 1/Snare2_HitSN_v6_rr1_Mid.wav`,
    `${SKINS}/Snare Drum, Modern 1/Snare2_HitSN_v6_rr2_Mid.wav`,
  ],
  clap: [
    `${METALS}/Claps/Clap_rr1.wav`,
    `${METALS}/Claps/Clap_rr2.wav`,
    `${METALS}/Claps/Clap_rr3.wav`,
  ],
  closedHat: [
    `${METALS}/Hi-Hat Cymbal/HiHat_HitC_v3_rr1_Mid.wav`,
    `${METALS}/Hi-Hat Cymbal/HiHat_HitC_v3_rr2_Mid.wav`,
  ],
  openHat: [
    `${METALS}/Hi-Hat Cymbal/HiHat_HitO_rr1_Mid.wav`,
    `${METALS}/Hi-Hat Cymbal/HiHat_HitO_rr2_Mid.wav`,
  ],
  lowTom: [
    `${SKINS}/Tom 2/Stick/TomL_HitS_v3_rr1_Mid.wav`,
    `${SKINS}/Tom 2/Stick/TomL_HitS_v3_rr2_Mid.wav`,
  ],
  highTom: [
    `${SKINS}/Tom 1/Stick/TomH_HitS_v3_rr1_Mid.wav`,
    `${SKINS}/Tom 1/Stick/TomH_HitS_v3_rr2_Mid.wav`,
  ],
  crash: [
    `${METALS}/Clash Cymbals 1/cymbal_crash1_mf1.wav`,
    `${METALS}/Clash Cymbals 1/cymbal_crash1_mf2.wav`,
  ],
  ride: [`${METALS}/Suspended Cymbal 1/susCymb1_hit_stick_mp1.wav`],
};

for (const [piece, info] of Object.entries(DRUM_PIECES)) {
  if (DRUM_SOURCES[piece as DrumPiece].length !== info.takes) {
    throw new Error(`drum kit: ${piece} lists a different number of takes`);
  }
}
const UPSTREAM_TIMEOUT_MS = 8000;
const PREFETCH_CONCURRENCY = 6;
/** FluidR3_GM is rendered from A0 to C8. */
export const LOWEST_SAMPLED_MIDI = 21;
export const HIGHEST_SAMPLED_MIDI = 108;

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
const INSTRUMENT_PATTERN = /^[a-z0-9_]+$/;
const NOTE_PATTERN = /^[A-G]b?[0-8]$/;

export function sampleNoteName(midi: number): string {
  return `${NOTE_NAMES[midi % 12]}${Math.floor(midi / 12) - 1}`;
}

export class SampleStore {
  private readonly directory: string;
  private readonly downloads = new Map<string, Promise<Buffer | null>>();

  constructor(directory: string) {
    this.directory = directory;
  }

  /**
   * The General MIDI program names, index = program. Cached next to the
   * samples; empty when the server has never been online.
   */
  async instrumentNames(): Promise<string[]> {
    const cached = join(this.directory, 'names.json');
    try {
      return JSON.parse(await readFile(cached, 'utf8')) as string[];
    } catch {
      // Not cached yet.
    }
    const body = await download(`${UPSTREAM}/names.json`);
    if (!body) {
      console.warn(
        'Samples: no instrument list (offline?); parts use synth voices'
      );
      return [];
    }
    await mkdir(this.directory, { recursive: true });
    await writeFile(cached, body);
    return JSON.parse(body.toString('utf8')) as string[];
  }

  /** One note's mp3, from the mirror or, the first time, from upstream. */
  get(instrument: string, note: string): Promise<Buffer | null> {
    if (!INSTRUMENT_PATTERN.test(instrument) || !NOTE_PATTERN.test(note)) {
      return Promise.resolve(null);
    }
    return this.mirrored(
      join(instrument, `${note}.mp3`),
      `${UPSTREAM}/${instrument}-mp3/${note}.mp3`
    );
  }

  /** One take (1-based) of a drum kit piece, as wav. */
  getDrum(piece: string, take: number): Promise<Buffer | null> {
    const source = DRUM_SOURCES[piece as DrumPiece]?.[take - 1];
    if (!source) {
      return Promise.resolve(null);
    }
    const url = `${VCSL}/${source.split('/').map(encodeURIComponent).join('/')}`;
    return this.mirrored(join(DRUMS, piece, `${take}.wav`), url);
  }

  private async mirrored(
    relativePath: string,
    url: string
  ): Promise<Buffer | null> {
    const path = join(this.directory, relativePath);
    try {
      return await readFile(path);
    } catch {
      // Not mirrored yet.
    }

    let pending = this.downloads.get(relativePath);
    if (!pending) {
      pending = download(url).then(async (body) => {
        if (body) {
          await mkdir(dirname(path), { recursive: true });
          await writeFile(path, body);
        }
        this.downloads.delete(relativePath);
        return body;
      });
      this.downloads.set(relativePath, pending);
    }
    return pending;
  }

  /** Mirrors every note any part of these scores plays, a few at a time. */
  async prefetch(scores: readonly Score[]): Promise<void> {
    const wanted = new Set<string>();
    for (const score of scores) {
      for (const part of score.parts) {
        if (part.instrument === DRUMS) {
          for (const note of part.notes) {
            const piece = drumPieceFor(note.midi);
            for (
              let take = 1;
              piece && take <= DRUM_PIECES[piece].takes;
              take += 1
            ) {
              wanted.add(`${DRUMS}/${piece}/${take}`);
            }
          }
          continue;
        }
        if (!part.instrument) {
          continue;
        }
        for (const note of part.notes) {
          const midi = Math.min(
            HIGHEST_SAMPLED_MIDI,
            Math.max(LOWEST_SAMPLED_MIDI, note.midi)
          );
          wanted.add(`${part.instrument}/${sampleNoteName(midi)}`);
        }
      }
    }

    const queue = [...wanted];
    let missing = 0;
    const worker = async () => {
      for (let key = queue.pop(); key; key = queue.pop()) {
        const [instrument, note, take] = key.split('/');
        const body =
          instrument === DRUMS
            ? await this.getDrum(note, Number(take))
            : await this.get(instrument, note);
        if (!body) {
          missing += 1;
        }
      }
    };
    await Promise.all(Array.from({ length: PREFETCH_CONCURRENCY }, worker));
    console.log(
      `Samples: ${wanted.size - missing}/${wanted.size} notes mirrored in ./samples` +
        (missing ? ` (${missing} unavailable)` : '')
    );
  }
}

async function download(url: string): Promise<Buffer | null> {
  try {
    const response = await fetch(url, {
      signal: AbortSignal.timeout(UPSTREAM_TIMEOUT_MS),
    });
    return response.ok ? Buffer.from(await response.arrayBuffer()) : null;
  } catch {
    return null;
  }
}
