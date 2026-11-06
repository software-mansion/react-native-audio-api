// The recorded drum kit: General MIDI percussion keys (channel 10) mapped to
// the kit pieces this server mirrors from VCSL. The phones import it too, so
// it stays free of imports and of TypeScript that Node cannot erase; it lives
// here because only this package is an ES module for Node.

/** `ScorePart.instrument` of a part played on the recorded kit. */
export const DRUMS = 'drums';

export type DrumPiece =
  | 'kick'
  | 'snare'
  | 'clap'
  | 'closedHat'
  | 'openHat'
  | 'lowTom'
  | 'highTom'
  | 'crash'
  | 'ride';

interface DrumPieceInfo {
  label: string;
  /** General MIDI percussion keys this piece plays. */
  keys: readonly number[];
  /** Recorded takes, played in random order so repeated hits differ. */
  takes: number;
  /** Cymbals fall back to the synth hat, skins to the synth clap. */
  metal: boolean;
}

export const DRUM_PIECES: Record<DrumPiece, DrumPieceInfo> = {
  kick: { label: 'kick', keys: [35, 36], takes: 2, metal: false },
  snare: { label: 'snare', keys: [37, 38, 40], takes: 2, metal: false },
  clap: { label: 'clap', keys: [39], takes: 3, metal: false },
  closedHat: { label: 'hi-hat', keys: [42, 44], takes: 2, metal: true },
  openHat: { label: 'open hat', keys: [46], takes: 2, metal: true },
  lowTom: { label: 'low tom', keys: [41, 43, 45], takes: 2, metal: false },
  highTom: { label: 'high tom', keys: [47, 48, 50], takes: 2, metal: false },
  crash: { label: 'crash', keys: [49, 52, 55, 57], takes: 2, metal: true },
  ride: { label: 'ride', keys: [51, 53, 59], takes: 1, metal: true },
};

const PIECE_BY_KEY = new Map<number, DrumPiece>();
for (const [piece, info] of Object.entries(DRUM_PIECES)) {
  for (const key of info.keys) {
    PIECE_BY_KEY.set(key, piece as DrumPiece);
  }
}

/**
 * The kit piece a General MIDI percussion key plays; null when the kit has
 * none.
 */
export function drumPieceFor(key: number): DrumPiece | null {
  return PIECE_BY_KEY.get(key) ?? null;
}
