import { blackKeyWidth, keyboardWidth, pianoKeys, whiteKeyWidth } from './keys';
import type { ScoreNote } from './usePianoDevApi';

/** Seconds between the keyboard positions the track stores. */
export const followSampleInterval = 0.1;

/** The keyboard looks ahead so it arrives before the notes do. */
const lookAhead = 1.2;
const lookBehind = 0.3;
/** Short notes still count for at least this long. */
const minimumNoteWeightDuration = 0.25;
/** Offsets the keyboard may settle on: every half white key. */
const candidateStep = whiteKeyWidth / 2;
/** Notes this close to the screen edge count as cut off. */
const edgeMargin = whiteKeyWidth / 4;
/**
 * Share of the best coverage a position may lose and still count as showing the
 * melody; lets the keyboard stay put instead of chasing small gains.
 */
const coverageTolerance = 0.1;

interface WeightedNote {
  left: number;
  right: number;
  start: number;
  end: number;
  importance: number;
}

/**
 * Precomputes where the keyboard should be scrolled while a score plays: one
 * offset every `followSampleInterval` seconds of score time.
 *
 * Each note weighs its loudness squared, so the melody's top voice (velocity 1)
 * outweighs the inner voices and the accompaniment, times how much of it sounds
 * within the next `lookAhead` seconds. The keyboard keeps its position while
 * that still shows nearly the heaviest set of keys. Otherwise it moves to the
 * position that does, closest to centring the notes' weighted mean.
 */
export function buildFollowTrack(
  score: ScoreNote[],
  viewportWidth: number
): number[] {
  const maxOffset = Math.max(0, keyboardWidth - viewportWidth);
  if (score.length === 0 || viewportWidth <= 0) {
    return [];
  }

  const keyByMidi = new Map(pianoKeys.map((key) => [key.midi, key]));
  const notes: WeightedNote[] = score.flatMap((note) => {
    const key = keyByMidi.get(note.midi);
    if (!key) {
      return [];
    }
    const width = key.isBlack ? blackKeyWidth : whiteKeyWidth;
    return [
      {
        left: key.x,
        right: key.x + width,
        start: note.time,
        end: note.time + Math.max(note.duration, minimumNoteWeightDuration),
        importance: (note.velocity ?? 1) ** 2,
      },
    ];
  });
  notes.sort((a, b) => a.start - b.start);

  const candidates: number[] = [];
  for (let offset = 0; offset < maxOffset; offset += candidateStep) {
    candidates.push(offset);
  }
  candidates.push(maxOffset);

  const scoreLength = Math.max(...notes.map((note) => note.end));
  const track: (number | null)[] = [];
  let position: number | null = null;
  let firstUnfinished = 0;

  for (let time = 0; time <= scoreLength; time += followSampleInterval) {
    const windowStart = time - lookBehind;
    const windowEnd = time + lookAhead;

    while (
      firstUnfinished < notes.length &&
      notes[firstUnfinished].end < windowStart
    ) {
      firstUnfinished += 1;
    }

    const weighted: { note: WeightedNote; weight: number }[] = [];
    for (let i = firstUnfinished; i < notes.length; i += 1) {
      const note = notes[i];
      if (note.start > windowEnd) {
        break;
      }
      const overlap =
        Math.min(note.end, windowEnd) - Math.max(note.start, windowStart);
      if (overlap > 0) {
        weighted.push({ note, weight: overlap * note.importance });
      }
    }

    if (weighted.length > 0) {
      const coverageAt = (offset: number) =>
        weighted.reduce(
          (sum, { note, weight }) =>
            note.left >= offset + edgeMargin &&
            note.right <= offset + viewportWidth - edgeMargin
              ? sum + weight
              : sum,
          0
        );
      const totalWeight = weighted.reduce((sum, { weight }) => sum + weight, 0);
      const coverages = candidates.map(coverageAt);
      const enough = Math.max(...coverages) - coverageTolerance * totalWeight;

      if (position === null || coverageAt(position) < enough) {
        const meanX =
          weighted.reduce(
            (sum, { note, weight }) =>
              sum + ((note.left + note.right) / 2) * weight,
            0
          ) / totalWeight;
        const centred = meanX - viewportWidth / 2;
        position = candidates
          .filter((_, i) => coverages[i] >= enough)
          .reduce((best, offset) =>
            Math.abs(offset - centred) < Math.abs(best - centred)
              ? offset
              : best
          );
      }
    }

    track.push(position);
  }

  // Before the first note there is nothing to follow; wait where it will play.
  const firstPosition = track.find((offset) => offset !== null) ?? 0;
  return track.map((offset) => offset ?? firstPosition);
}
