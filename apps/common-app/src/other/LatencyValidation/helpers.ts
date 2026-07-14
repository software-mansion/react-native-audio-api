export async function sleep(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

export function formatMs(seconds: number): string {
  return `${(seconds * 1000).toFixed(2)} ms`;
}

export function formatTimestamp(timestamp: number): string {
  return new Date(timestamp).toLocaleTimeString([], {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  });
}

export interface BeepPattern {
  samples: Float32Array;
  durationSeconds: number;
  beepCount: number;
  frequenciesHz: number[];
}

const BEEP_PATTERN = {
  beepDurationMs: 45,
  gapDurationMs: 70,
  peakAmplitude: 0.85,
  frequenciesHz: [880, 1046, 1244, 1046, 880],
} as const;

const PLOT_MAX_POINTS = 320;
const ALIGNMENT_BLOCK_SIZE = 128;
const BASELINE_WINDOW_SECONDS = 0.15;

function msToSamples(durationMs: number, sampleRate: number): number {
  return Math.max(1, Math.round((durationMs / 1000) * sampleRate));
}

function createHannWindow(length: number): Float32Array {
  const window = new Float32Array(length);

  if (length <= 1) {
    window[0] = 1;
    return window;
  }

  const denominator = length - 1;
  for (let index = 0; index < length; index += 1) {
    window[index] = 0.5 * (1 - Math.cos((2 * Math.PI * index) / denominator));
  }

  return window;
}

function writeBeepTone(
  destination: Float32Array,
  offset: number,
  frequencyHz: number,
  sampleRate: number,
  beepSamples: number,
  envelope: Float32Array,
  peakAmplitude: number
): void {
  const phaseIncrement = (2 * Math.PI * frequencyHz) / sampleRate;

  for (let index = 0; index < beepSamples; index += 1) {
    destination[offset + index] =
      peakAmplitude * envelope[index] * Math.sin(phaseIncrement * index);
  }
}

export function buildBeepReferencePattern(sampleRate: number): BeepPattern {
  const { beepDurationMs, gapDurationMs, peakAmplitude, frequenciesHz } =
    BEEP_PATTERN;
  const beepSamples = msToSamples(beepDurationMs, sampleRate);
  const gapSamples = msToSamples(gapDurationMs, sampleRate);
  const beepCount = frequenciesHz.length;
  const totalSamples = beepCount * beepSamples + (beepCount - 1) * gapSamples;
  const samples = new Float32Array(totalSamples);
  const envelope = createHannWindow(beepSamples);

  let offset = 0;
  for (let beepIndex = 0; beepIndex < beepCount; beepIndex += 1) {
    writeBeepTone(
      samples,
      offset,
      frequenciesHz[beepIndex],
      sampleRate,
      beepSamples,
      envelope,
      peakAmplitude
    );

    offset += beepSamples;
    if (beepIndex < beepCount - 1) {
      offset += gapSamples;
    }
  }

  return {
    samples,
    durationSeconds: totalSamples / sampleRate,
    beepCount,
    frequenciesHz: [...frequenciesHz],
  };
}

export function appendChannelSamples(
  recorded: number[],
  buffer: {
    numberOfChannels: number;
    getChannelData: (channel: number) => Float32Array;
  },
  channel = 0
): void {
  const data = buffer.getChannelData(channel);
  for (let index = 0; index < data.length; index += 1) {
    recorded.push(data[index]);
  }
}

export function estimateNoiseFloor(samples: number[], windowLength: number): number {
  const end = Math.min(samples.length, windowLength);
  if (end === 0) {
    return 0;
  }

  let sumSquares = 0;
  for (let index = 0; index < end; index += 1) {
    sumSquares += samples[index] * samples[index];
  }

  return Math.sqrt(sumSquares / end);
}

export function prepareRecordedSamples(
  samples: number[],
  sampleRate: number
): {
  processed: Float32Array;
  noiseFloor: number;
  baselineLength: number;
} {
  const baselineLength = Math.min(
    samples.length,
    Math.round(BASELINE_WINDOW_SECONDS * sampleRate)
  );
  const noiseFloor = estimateNoiseFloor(samples, baselineLength);

  let mean = 0;
  for (let index = 0; index < baselineLength; index += 1) {
    mean += samples[index];
  }
  mean /= Math.max(baselineLength, 1);

  const gateThreshold = Math.max(noiseFloor * 2.5, 0.002);
  const processed = new Float32Array(samples.length);

  for (let index = 0; index < samples.length; index += 1) {
    const centered = samples[index] - mean;
    processed[index] =
      Math.abs(centered) < gateThreshold ? 0 : centered;
  }

  return {
    processed,
    noiseFloor,
    baselineLength,
  };
}

export function downsampleEnvelopeForPlot(
  samples: Float32Array | number[],
  maxPoints = PLOT_MAX_POINTS
): number[] {
  if (samples.length === 0) {
    return [];
  }

  const blockSize = Math.max(1, Math.ceil(samples.length / maxPoints));
  const points: number[] = [];

  for (let blockIndex = 0; blockIndex < maxPoints; blockIndex += 1) {
    const start = blockIndex * blockSize;
    if (start >= samples.length) {
      break;
    }

    const end = Math.min(samples.length, start + blockSize);
    let peak = 0;

    for (let index = start; index < end; index += 1) {
      peak = Math.max(peak, Math.abs(samples[index]));
    }

    points.push(peak);
  }

  return points;
}

export function sliceSamples(
  samples: Float32Array | number[],
  startSample: number,
  length: number
): Float32Array {
  const end = Math.min(samples.length, startSample + length);
  const slice = new Float32Array(Math.max(0, end - startSample));

  for (let index = startSample; index < end; index += 1) {
    slice[index - startSample] = samples[index];
  }

  return slice;
}

function downsampleEnvelope(
  samples: Float32Array | number[],
  blockSize: number
): Float32Array {
  const length = Math.ceil(samples.length / blockSize);
  const envelope = new Float32Array(length);

  for (let blockIndex = 0; blockIndex < length; blockIndex += 1) {
    const start = blockIndex * blockSize;
    const end = Math.min(samples.length, start + blockSize);
    let peak = 0;

    for (let index = start; index < end; index += 1) {
      peak = Math.max(peak, Math.abs(samples[index]));
    }

    envelope[blockIndex] = peak;
  }

  return envelope;
}

function normalizedCorrelation(
  reference: Float32Array,
  recorded: Float32Array,
  lag: number
): number {
  const overlap = Math.min(reference.length, recorded.length - lag);
  if (overlap <= 0) {
    return -1;
  }

  let dot = 0;
  let refEnergy = 0;
  let recEnergy = 0;

  for (let index = 0; index < overlap; index += 1) {
    const refSample = reference[index];
    const recSample = recorded[lag + index];
    dot += refSample * recSample;
    refEnergy += refSample * refSample;
    recEnergy += recSample * recSample;
  }

  const norm = Math.sqrt(refEnergy * recEnergy);
  return norm > 0 ? dot / norm : -1;
}

export interface CorrelationPoint {
  lagMs: number;
  score: number;
}

export interface AlignmentResult {
  lagSamples: number;
  correlation: number;
  lagSeconds: number;
  coarseLagSamples: number;
  coarseCorrelation: number;
  correlationProfile: CorrelationPoint[];
  searchStartLagSamples: number;
  searchEndLagSamples: number;
}

export function findPatternAlignment(
  reference: Float32Array,
  recordedSamples: Float32Array | number[],
  sampleRate: number,
  expectedLagSeconds?: number
): AlignmentResult | null {
  if (recordedSamples.length < reference.length) {
    return null;
  }

  const recorded =
    recordedSamples instanceof Float32Array
      ? recordedSamples
      : Float32Array.from(recordedSamples);
  const blockSize = ALIGNMENT_BLOCK_SIZE;
  const refEnvelope = downsampleEnvelope(reference, blockSize);
  const recEnvelope = downsampleEnvelope(recorded, blockSize);
  const maxBlockLag = recEnvelope.length - refEnvelope.length;

  if (maxBlockLag < 0) {
    return null;
  }

  const expectedBlockLag =
    expectedLagSeconds !== undefined
      ? Math.round((expectedLagSeconds * sampleRate) / blockSize)
      : null;
  const searchRadiusBlocks = Math.ceil((0.8 * sampleRate) / blockSize);
  const minBlockLag = Math.max(
    0,
    expectedBlockLag !== null ? expectedBlockLag - searchRadiusBlocks : 0
  );
  const maxBlockLagSearch = Math.min(
    maxBlockLag,
    expectedBlockLag !== null
      ? expectedBlockLag + searchRadiusBlocks
      : maxBlockLag
  );

  let bestBlockLag = 0;
  let bestBlockScore = -1;
  const correlationProfile: CorrelationPoint[] = [];

  for (let blockLag = minBlockLag; blockLag <= maxBlockLagSearch; blockLag += 1) {
    const score = normalizedCorrelation(refEnvelope, recEnvelope, blockLag);
    correlationProfile.push({
      lagMs: ((blockLag * blockSize) / sampleRate) * 1000,
      score,
    });

    if (score > bestBlockScore) {
      bestBlockScore = score;
      bestBlockLag = blockLag;
    }
  }

  if (bestBlockScore < 0.2) {
    correlationProfile.length = 0;
    let fallbackBestLag = 0;
    let fallbackBestScore = -1;

    for (let blockLag = 0; blockLag <= maxBlockLag; blockLag += 1) {
      const score = normalizedCorrelation(refEnvelope, recEnvelope, blockLag);
      correlationProfile.push({
        lagMs: ((blockLag * blockSize) / sampleRate) * 1000,
        score,
      });

      if (score > fallbackBestScore) {
        fallbackBestScore = score;
        fallbackBestLag = blockLag;
      }
    }

    if (fallbackBestScore < 0.25) {
      return null;
    }

    bestBlockLag = fallbackBestLag;
    bestBlockScore = fallbackBestScore;
  }

  const coarseLagSamples = bestBlockLag * blockSize;
  const refineRadius = blockSize * 2;
  const minSampleLag = Math.max(0, coarseLagSamples - refineRadius);
  const maxSampleLag = Math.min(
    recorded.length - reference.length,
    coarseLagSamples + refineRadius
  );

  let bestLagSamples = coarseLagSamples;
  let bestSampleScore = -1;

  for (let lag = minSampleLag; lag <= maxSampleLag; lag += 1) {
    const score = normalizedCorrelation(reference, recorded, lag);
    if (score > bestSampleScore) {
      bestSampleScore = score;
      bestLagSamples = lag;
    }
  }

  return {
    lagSamples: bestLagSamples,
    correlation: bestSampleScore,
    lagSeconds: bestLagSamples / sampleRate,
    coarseLagSamples,
    coarseCorrelation: bestBlockScore,
    correlationProfile,
    searchStartLagSamples: minBlockLag * blockSize,
    searchEndLagSamples: maxBlockLagSearch * blockSize,
  };
}

export function buildRecordedWindow(
  recordedSamples: Float32Array | number[],
  centerLagSamples: number,
  windowSeconds: number,
  sampleRate: number
): Float32Array {
  const windowSamples = Math.round(windowSeconds * sampleRate);
  const startSample = Math.max(0, centerLagSamples - Math.floor(windowSamples / 2));
  return sliceSamples(recordedSamples, startSample, windowSamples);
}
