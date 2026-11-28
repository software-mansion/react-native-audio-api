/** `m:ss`, or `mm:ss` with `padMinutes`. Negative input clamps to zero. */
export function formatClock(seconds: number, padMinutes = false) {
  'worklet';
  const whole = Math.max(0, Math.floor(seconds));
  const minutes = String(Math.floor(whole / 60));
  const rest = String(whole % 60).padStart(2, '0');
  return `${padMinutes ? minutes.padStart(2, '0') : minutes}:${rest}`;
}
