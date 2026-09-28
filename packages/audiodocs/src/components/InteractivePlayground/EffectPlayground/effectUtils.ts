import type { AudioContext, AudioParam } from 'react-native-audio-api';

const PARAM_SMOOTHING_SECONDS = 0.02;

/**
 * Moves an AudioParam towards `value` over a few milliseconds instead of jumping, so dragging a
 * slider does not produce zipper noise.
 */
export function setParamSmoothly(param: AudioParam, value: number, ctx: AudioContext): void {
  param.setTargetAtTime(value, ctx.currentTime, PARAM_SMOOTHING_SECONDS);
}

export function formatFrequency(hz: number): string {
  if (hz >= 1000) {
    return `${(hz / 1000).toFixed(hz >= 10000 ? 0 : 1)} kHz`;
  }
  return `${Math.round(hz)} Hz`;
}
