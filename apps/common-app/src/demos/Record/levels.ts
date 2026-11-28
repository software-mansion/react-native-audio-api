/** Quietest level drawn above a dot, in dBFS. */
const floorDb = -40;

/** Maps a peak amplitude (0–1) onto 0–1 over the last `-floorDb` decibels. */
export function levelFromAmplitude(amplitude: number) {
  'worklet';
  if (amplitude <= 0) {
    return 0;
  }
  const db = 20 * Math.log10(amplitude);
  return Math.max(0, Math.min(1, (db - floorDb) / -floorDb));
}
