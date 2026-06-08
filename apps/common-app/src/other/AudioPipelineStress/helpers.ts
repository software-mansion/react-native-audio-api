import { POSITION_POLL_INTERVAL_MS } from './constants';
import type { PlaybackProgressStats, SerializedError } from './types';

export async function sleep(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

export function formatDurationMs(durationMs: number): string {
  return `${(durationMs / 1000).toFixed(2)}s`;
}

export function formatTimestamp(timestamp: number): string {
  const date = new Date(timestamp);
  return date.toLocaleTimeString([], {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  });
}

export function getPlaybackProgressThreshold(durationSeconds: number): number {
  return Number(
    Math.max(0.08, Math.min(durationSeconds * 0.15, 0.18)).toFixed(2)
  );
}

export function serializeUnknownError(
  error: unknown,
  contextLabel?: string
): SerializedError {
  const prefix = contextLabel ? `${contextLabel}: ` : '';

  if (error instanceof Error) {
    const headline = `${prefix}${error.name}: ${error.message}`;
    const details = [headline, error.stack].filter(Boolean).join('\n');

    return { headline, details };
  }

  const type = typeof error;
  const constructorName =
    error && typeof error === 'object' && 'constructor' in error
      ? ((error as { constructor?: { name?: string } }).constructor?.name ??
        'unknown')
      : 'n/a';
  const keys =
    error && typeof error === 'object' ? Object.keys(error as object) : [];

  let renderedValue = '';

  try {
    renderedValue =
      typeof error === 'string' ? error : JSON.stringify(error, null, 2);
  } catch {
    renderedValue = String(error);
  }

  const headline = `${prefix}Non-Error throw: ${String(error)}`;
  const details = [
    headline,
    `typeof=${type}`,
    `constructor=${constructorName}`,
    `keys=${keys.join(', ') || 'none'}`,
    `value=${renderedValue}`,
  ].join('\n');

  return { headline, details };
}

export function formatPlaybackProgressStats(
  stats: PlaybackProgressStats
): string {
  const diagnosis =
    stats.engineDeltaSeconds < 0.05
      ? 'engine-clock-stalled'
      : stats.positionMaxObservedSeconds < stats.thresholdSeconds
        ? 'engine-running-node-stalled'
        : 'engine-and-node-running';

  return [
    `diagnosis=${diagnosis}`,
    `threshold=${stats.thresholdSeconds.toFixed(2)}s`,
    `positionMax=${stats.positionMaxObservedSeconds.toFixed(3)}s`,
    `engineDelta=${stats.engineDeltaSeconds.toFixed(3)}s`,
    `engineStart=${stats.engineStartTimeSeconds.toFixed(3)}s`,
    `engineEnd=${stats.engineEndTimeSeconds.toFixed(3)}s`,
    `recentPositionSamples=[${stats.positionSamples
      .slice(-8)
      .map((value) => value.toFixed(3))
      .join(', ')}]`,
    `recentEngineSamples=[${stats.engineSamples
      .slice(-8)
      .map((value) => value.toFixed(3))
      .join(', ')}]`,
  ].join(', ');
}

export async function waitForCondition(
  condition: () => boolean,
  timeoutMs: number,
  failureMessage: string,
  intervalMs: number = POSITION_POLL_INTERVAL_MS
): Promise<void> {
  const startedAt = Date.now();

  while (Date.now() - startedAt < timeoutMs) {
    if (condition()) {
      return;
    }

    await sleep(intervalMs);
  }

  throw new Error(failureMessage);
}
