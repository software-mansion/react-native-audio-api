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

/** `m:ss`, the way recording lengths and ruler marks are written. */
export function formatDuration(seconds: number) {
  'worklet';
  const whole = Math.floor(seconds);
  const minutes = Math.floor(whole / 60);
  const rest = whole % 60;
  return `${minutes}:${rest < 10 ? '0' : ''}${rest}`;
}
