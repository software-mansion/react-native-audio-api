// Maps `performance.now()` onto `AudioContext.currentTime`, so a moment agreed
// on the network can be handed to `start(when)` and be *heard* at that moment.
//
// Two sources, selectable for A/B comparison:
//
// - 'outputTimestamp': `getOutputTimestamp()` pairs the context time of the
//   sample leaving the speaker with the `performance.now()` moment it does so.
//   Output latency is already inside the pair, and the native side reads it at
//   audio-callback precision, so the window median is the estimate.
//
// - 'currentTime': `currentTime` only advances once per hardware buffer, so a
//   (perf, currentTime) reading can be up to one buffer late but never early.
//   The largest currentTime − perf ("lead") in the window is the tightest
//   estimate of what is being *rendered*; `outputLatency` is then subtracted.
//
// Windows are short so clock drift between the audio and system clocks never
// accumulates.
import type { AudioContext } from 'react-native-audio-api';

export type AudioClockSource = 'outputTimestamp' | 'currentTime';

export interface AudioClockQuality {
  /** The source actually in use (falls back to 'currentTime'). */
  source: AudioClockSource;
  /** How much the estimate moves between refreshes (median absolute deviation). */
  jitterMs: number;
}

interface LeadSample {
  perf: number;
  leadMs: number;
}

const WINDOW_MS = 3000;
const SAMPLE_INTERVAL_MS = 4;
const ESTIMATE_HISTORY = 40;
const ESTIMATE_INTERVAL_MS = 250;
const MIN_SAMPLES = 8;

export default class AudioClockMap {
  private readonly context: AudioContext;
  private preferred: AudioClockSource;
  private currentTimeLeads: LeadSample[] = [];
  private outputTimestampLeads: LeadSample[] = [];
  private estimates: number[] = [];
  private estimateSource: AudioClockSource | null = null;
  private lastEstimateAt = 0;
  private outputTimestampSupported = true;
  private timer: ReturnType<typeof setInterval> | null = null;

  constructor(context: AudioContext, preferred: AudioClockSource) {
    this.context = context;
    this.preferred = preferred;
  }

  setPreferredSource(source: AudioClockSource): void {
    this.preferred = source;
  }

  start(): void {
    if (this.timer) {
      return;
    }
    this.timer = setInterval(() => this.sample(), SAMPLE_INTERVAL_MS);
  }

  stop(): void {
    if (this.timer) {
      clearInterval(this.timer);
      this.timer = null;
    }
    this.currentTimeLeads = [];
    this.outputTimestampLeads = [];
    this.estimates = [];
  }

  get isReady(): boolean {
    return this.currentTimeLeads.length >= MIN_SAMPLES;
  }

  get quality(): AudioClockQuality {
    return { source: this.activeSource(), jitterMs: this.estimateJitter() };
  }

  /** Context time to pass to `start()` for the sound to be heard at `perfMs`. */
  startTimeHeardAt(perfMs: number): number {
    if (!this.currentTimeLeads.length) {
      this.sample();
    }
    return (perfMs + this.heardLeadMs()) / 1000;
  }

  private activeSource(): AudioClockSource {
    const outputTimestampReady =
      this.outputTimestampSupported &&
      this.outputTimestampLeads.length >= MIN_SAMPLES;
    return this.preferred === 'outputTimestamp' && outputTimestampReady
      ? 'outputTimestamp'
      : 'currentTime';
  }

  // contextTime (ms) − perf of the sound being *heard*.
  private heardLeadMs(): number {
    if (this.activeSource() === 'outputTimestamp') {
      return median(this.outputTimestampLeads.map((s) => s.leadMs));
    }
    const renderedLeadMs = this.currentTimeLeads.reduce(
      (best, s) => Math.max(best, s.leadMs),
      Number.NEGATIVE_INFINITY
    );
    return renderedLeadMs - this.context.outputLatency * 1000;
  }

  private sample(): void {
    const perf = performance.now();
    const horizon = perf - WINDOW_MS;

    this.currentTimeLeads.push({
      perf,
      leadMs: this.context.currentTime * 1000 - perf,
    });
    trim(this.currentTimeLeads, horizon);

    if (this.outputTimestampSupported) {
      try {
        const { contextTime, performanceTime } =
          this.context.getOutputTimestamp();
        // Zero until the first block has been rendered.
        if (contextTime > 0) {
          this.outputTimestampLeads.push({
            perf,
            leadMs: contextTime * 1000 - performanceTime,
          });
        }
      } catch {
        // A native build that predates getOutputTimestamp().
        this.outputTimestampSupported = false;
      }
      trim(this.outputTimestampLeads, horizon);
    }

    this.recordEstimate(perf);
  }

  private recordEstimate(perf: number): void {
    if (perf - this.lastEstimateAt < ESTIMATE_INTERVAL_MS || !this.isReady) {
      return;
    }
    const source = this.activeSource();
    if (source !== this.estimateSource) {
      this.estimates = [];
      this.estimateSource = source;
    }
    this.lastEstimateAt = perf;
    this.estimates.push(this.heardLeadMs());
    if (this.estimates.length > ESTIMATE_HISTORY) {
      this.estimates.shift();
    }
  }

  private estimateJitter(): number {
    if (this.estimates.length < 2) {
      return Number.NaN;
    }
    const center = median(this.estimates);
    return median(this.estimates.map((e) => Math.abs(e - center)));
  }
}

function trim(samples: LeadSample[], horizon: number): void {
  while (samples.length && samples[0].perf < horizon) {
    samples.shift();
  }
}

function median(values: number[]): number {
  const sorted = [...values].sort((a, b) => a - b);
  return sorted[Math.floor(sorted.length / 2)];
}
