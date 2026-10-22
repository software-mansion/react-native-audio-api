import type { AudioContext } from 'react-native-audio-api';

export type SchedulingMethod =
  | 'outputTimestamp'
  | 'latencyCompensated'
  | 'currentTime';
/** `alternate` interleaves `outputTimestamp` and `latencyCompensated`. */
export type SchedulingMode = SchedulingMethod | 'alternate';

export const SCHEDULING_METHODS: SchedulingMethod[] = [
  'outputTimestamp',
  'latencyCompensated',
  'currentTime',
];
export const SCHEDULING_MODES: SchedulingMode[] = [
  'alternate',
  ...SCHEDULING_METHODS,
];

/** How far ahead of `performance.now()` each click and flash are aimed. */
export const LEAD_MS = 500;
export const TRIAL_PERIOD_MS = 1500;
/** Random extra delay so triggers land at every phase of the audio callback. */
export const TRIAL_JITTER_MS = 100;

/** Pitch tells the methods apart on a recording made in `alternate` mode. */
export const CLICK_FREQUENCY_HZ: Record<SchedulingMethod, number> = {
  outputTimestamp: 2000,
  latencyCompensated: 1000,
  currentTime: 500,
};

export interface Trial {
  index: number;
  method: SchedulingMethod;
  targetPerfMs: number;
  /** Context time the click was started at. */
  when: number;
  /**
   * Where each method would put the click, relative to `outputTimestamp`, in
   * ms.
   */
  offsetFromTimestampMs: Record<SchedulingMethod, number>;
  /** `currentTime - contextTime` when scheduling: the audible lag of the graph. */
  renderLeadMs: number;
  outputLatencyMs: number;
  /** When the flash was actually triggered, relative to the target. */
  flashErrorMs?: number;
}

export function methodForTrial(
  mode: SchedulingMode,
  index: number
): SchedulingMethod {
  if (mode !== 'alternate') {
    return mode;
  }
  return index % 2 === 0 ? 'outputTimestamp' : 'latencyCompensated';
}

/**
 * Context time of the click for every method. `null` until the context has
 * rendered its first block and `getOutputTimestamp()` has a real pair.
 */
export function scheduleTimes(context: AudioContext, targetPerfMs: number) {
  const { contextTime, performanceTime } = context.getOutputTimestamp();
  if (performanceTime === 0) {
    return null;
  }

  const leadSeconds = (targetPerfMs - performance.now()) / 1000;
  const outputLatency = context.outputLatency;
  const when: Record<SchedulingMethod, number> = {
    outputTimestamp: contextTime + (targetPerfMs - performanceTime) / 1000,
    latencyCompensated: context.currentTime + leadSeconds - outputLatency,
    currentTime: context.currentTime + leadSeconds,
  };

  return {
    when,
    renderLeadMs: (context.currentTime - contextTime) * 1000,
    outputLatencyMs: outputLatency * 1000,
  };
}

/** A tone burst starting at full amplitude, so the onset is sharp on a waveform. */
export function createClickBuffer(context: AudioContext, frequencyHz: number) {
  const durationSeconds = 0.02;
  const length = Math.round(context.sampleRate * durationSeconds);
  const buffer = context.createBuffer(1, length, context.sampleRate);
  const samples = new Float32Array(length);
  const decayPerSecond = 150;

  for (let i = 0; i < length; i++) {
    const t = i / context.sampleRate;
    samples[i] =
      Math.cos(2 * Math.PI * frequencyHz * t) * Math.exp(-decayPerSecond * t);
  }
  buffer.copyToChannel(samples, 0);
  return buffer;
}

export function meanAndSpread(values: number[]) {
  if (values.length === 0) {
    return { mean: 0, spread: 0 };
  }
  const mean = values.reduce((sum, v) => sum + v, 0) / values.length;
  const variance =
    values.reduce((sum, v) => sum + (v - mean) ** 2, 0) / values.length;
  return { mean, spread: Math.sqrt(variance) };
}

export const CSV_HEADER =
  'index,method,targetPerfMs,when,latencyCompensatedOffsetMs,currentTimeOffsetMs,renderLeadMs,outputLatencyMs,flashErrorMs';

export function toCsvRow(trial: Trial) {
  return [
    trial.index,
    trial.method,
    trial.targetPerfMs.toFixed(3),
    trial.when.toFixed(6),
    trial.offsetFromTimestampMs.latencyCompensated.toFixed(3),
    trial.offsetFromTimestampMs.currentTime.toFixed(3),
    trial.renderLeadMs.toFixed(3),
    trial.outputLatencyMs.toFixed(3),
    trial.flashErrorMs?.toFixed(3) ?? '',
  ].join(',');
}
