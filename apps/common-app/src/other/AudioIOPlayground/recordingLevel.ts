/**
 * Shown until the first captured buffer. A row of empty blocks means silence
 * arrived.
 */
export const NO_BUFFERS = 'no buffers';

/**
 * One callback is one column. At the requested quarter-second chunk. Newest
 * stays on the right.
 */
const COLUMN_COUNT = 16;

/**
 * Speech often peaks under 0.2, so a linear 0..1 scale would look like silence.
 * 0.25 is already the tallest block.
 */
const FULL_SCALE_PEAK = 0.25;

/** Below this the block stays empty, so a quiet room is not drawn as a signal. */
const SILENCE_PEAK = 0.01;

const LEVELS = '▁▂▃▄▅▆▇█';

export const LEVEL_CALLBACK_OPTIONS = {
  sampleRate: 64000,
  bufferLength: 4000,
  channelCount: 1,
};

export function peakAmplitude(samples: Float32Array): number {
  let peak = 0;
  for (let i = 0; i < samples.length; i++) {
    const amplitude = Math.abs(samples[i]);
    if (amplitude > peak) {
      peak = amplitude;
    }
  }
  return peak;
}

export function glyphForPeak(peak: number): string {
  if (peak < SILENCE_PEAK) {
    return LEVELS[0];
  }
  const scaled = Math.min(peak, FULL_SCALE_PEAK) / FULL_SCALE_PEAK;
  const index = Math.min(
    LEVELS.length - 1,
    Math.ceil(scaled * (LEVELS.length - 1))
  );
  return LEVELS[index];
}

/** Grows until {@link COLUMN_COUNT}, then drops the oldest column. */
export function appendLevel(previous: string, glyph: string): string {
  const row = previous === NO_BUFFERS ? '' : previous;
  return (row + glyph).slice(-COLUMN_COUNT);
}
