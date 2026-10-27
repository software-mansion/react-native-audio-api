// NTP-style estimate of the server clock as seen from this phone.
//
// Each ping/pong gives one sample: offset = server − local and the round-trip
// time. Samples taken while the network was congested are useless, so only
// the lowest-RTT samples in the window vote (median), and a slow linear drift
// between the two clocks is fitted once the window spans enough time.
import type { ServerMessage } from './protocol';

interface Sample {
  localTime: number;
  offset: number;
  rtt: number;
}

export interface ClockSyncQuality {
  /**
   * Server − local. Meaningless on its own: the two clocks start at different
   * moments.
   */
  offsetMs: number;
  rttMs: number;
  /**
   * Worst-case error of the offset: the reply may have spent anywhere in the
   * round trip in flight.
   */
  uncertaintyMs: number;
  /** Median absolute deviation of the voting samples' offsets. */
  jitterMs: number;
  driftPpm: number;
  samples: number;
}

const WINDOW_SIZE = 64;
/** Samples slower than best RTT + this margin do not vote. */
const RTT_MARGIN_MS = 3;
/** Drift is only trusted over a span long enough to rise above RTT jitter. */
const MIN_DRIFT_SPAN_MS = 20000;
const MAX_PLAUSIBLE_DRIFT_PPM = 500;

export default class ClockSync {
  private samples: Sample[] = [];
  private offsetMs = 0;
  private jitterMs = 0;
  private driftPpm = 0;
  private anchorLocalTime = 0;

  makePing(): { type: 'ping'; t0: number } {
    return { type: 'ping', t0: performance.now() };
  }

  acceptPong(pong: Extract<ServerMessage, { type: 'pong' }>): void {
    const t3 = performance.now();
    const { t0, t1, t2 } = pong;

    this.samples.push({
      localTime: t3,
      offset: (t1 - t0 + (t2 - t3)) / 2,
      rtt: t3 - t0 - (t2 - t1),
    });
    if (this.samples.length > WINDOW_SIZE) {
      this.samples.shift();
    }

    this.refit();
  }

  get isSynced(): boolean {
    return this.samples.length >= 4;
  }

  get quality(): ClockSyncQuality {
    const rttMs = this.bestRtt();
    return {
      offsetMs: this.offsetMs,
      rttMs,
      uncertaintyMs: rttMs / 2,
      jitterMs: this.jitterMs,
      driftPpm: this.driftPpm,
      samples: this.samples.length,
    };
  }

  localToServer(localMs: number): number {
    return localMs + this.offsetAt(localMs);
  }

  serverToLocal(serverMs: number): number {
    // offset changes by at most ~1 ms over any realistic session, so one
    // fixed-point step is enough to invert localToServer.
    const approximateLocal = serverMs - this.offsetMs;
    return serverMs - this.offsetAt(approximateLocal);
  }

  private offsetAt(localMs: number): number {
    return (
      this.offsetMs + ((localMs - this.anchorLocalTime) * this.driftPpm) / 1e6
    );
  }

  private bestRtt(): number {
    return this.samples.reduce(
      (best, sample) => Math.min(best, sample.rtt),
      Number.POSITIVE_INFINITY
    );
  }

  private refit(): void {
    const threshold = this.bestRtt() + RTT_MARGIN_MS;
    const voters = this.samples.filter((sample) => sample.rtt <= threshold);

    const offsets = voters.map((sample) => sample.offset).sort((a, b) => a - b);
    const latest = voters[voters.length - 1];
    this.offsetMs = median(offsets);
    this.jitterMs = median(
      offsets.map((offset) => Math.abs(offset - this.offsetMs))
    );
    this.anchorLocalTime = latest.localTime;

    const span = latest.localTime - voters[0].localTime;
    if (voters.length >= 8 && span >= MIN_DRIFT_SPAN_MS) {
      const slope = leastSquaresSlope(voters);
      this.driftPpm =
        Math.abs(slope * 1e6) <= MAX_PLAUSIBLE_DRIFT_PPM ? slope * 1e6 : 0;
    } else {
      this.driftPpm = 0;
    }
  }
}

function median(values: number[]): number {
  const sorted = [...values].sort((a, b) => a - b);
  return sorted[Math.floor(sorted.length / 2)];
}

function leastSquaresSlope(samples: Sample[]): number {
  const n = samples.length;
  const meanX = samples.reduce((sum, s) => sum + s.localTime, 0) / n;
  const meanY = samples.reduce((sum, s) => sum + s.offset, 0) / n;

  let covariance = 0;
  let variance = 0;
  for (const { localTime, offset } of samples) {
    covariance += (localTime - meanX) * (offset - meanY);
    variance += (localTime - meanX) ** 2;
  }

  return variance === 0 ? 0 : covariance / variance;
}
