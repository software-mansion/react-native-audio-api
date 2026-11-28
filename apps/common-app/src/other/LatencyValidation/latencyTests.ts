import {
  AudioContext,
  AudioManager,
  AudioRecorder,
} from 'react-native-audio-api';

import { recorderOptions } from '../../singletons';
import { colors } from '../../styles';
import type { ScenarioItem, StepItem } from '../../testComponents';
import { sleep } from '../sleep';
import {
  appendChannelSamples,
  buildBeepReferencePattern,
  buildRecordedWindow,
  downsampleEnvelopeForPlot,
  findPatternAlignment,
  formatMs,
  prepareRecordedSamples,
  type BeepPattern,
} from './helpers';
import type { LoopbackAnalysis, WaveformSeries } from './types';

// Keep the live full-duplex window (mic + speaker both active) as short as
// possible: the raw loopback path produces an idle buzz, so we only settle the
// recorder briefly and use a short scheduling lead before the beep plays.
const RECORDER_SETTLE_MS = 70;
const PLAY_LEAD_TIME_S = 0.07;
const LOOPBACK_TOLERANCE_MS = 120;
const MIN_CORRELATION = 0.45;
const PATTERN_WINDOW_PADDING_SECONDS = 0.12;

/**
 * The context and recorder of one run. They are created per run instead of
 * using the app's singletons, so stopping them never cuts off audio another
 * screen owns, e.g. a Record demo take that continues in the background.
 */
export interface LoopbackAudioIO {
  audioContext: AudioContext;
  audioRecorder: AudioRecorder;
}

export function createLoopbackAudioIO(): LoopbackAudioIO {
  return {
    audioContext: new AudioContext(),
    audioRecorder: new AudioRecorder(recorderOptions),
  };
}

async function stopLoopbackAudioIO({
  audioContext,
  audioRecorder,
}: LoopbackAudioIO): Promise<void> {
  audioRecorder.clearOnAudioReady();

  if (audioRecorder.isRecording()) {
    await audioRecorder.stop();
  }

  if (audioContext.state === 'running') {
    await audioContext.suspend();
  }
}

/**
 * Releases a run's audio I/O and deactivates the session the run activated.
 * Never throws, so the screen always leaves its running state.
 */
export async function closeLoopbackAudioIO(io: LoopbackAudioIO): Promise<void> {
  try {
    await stopLoopbackAudioIO(io);
    await io.audioContext.close();
  } catch (error) {
    console.warn('LatencyValidation: releasing audio I/O failed', error);
  }

  try {
    await AudioManager.setAudioSessionActivity(false);
  } catch (error) {
    console.warn('LatencyValidation: session deactivation failed', error);
  }
}

function envelopeSeries(
  label: string,
  color: string,
  samples: Float32Array | number[]
): WaveformSeries {
  return { label, color, points: downsampleEnvelopeForPlot(samples) };
}

function createScenario(startedAt: number, steps: StepItem[]): ScenarioItem {
  const status = steps.some((step) => step.status === 'fail')
    ? 'fail'
    : steps.every((step) => step.status === 'skipped')
      ? 'skipped'
      : 'pass';

  return {
    id: 'loopback-latency',
    title: 'Speaker-to-mic loopback check',
    status,
    durationLabel: `${((Date.now() - startedAt) / 1000).toFixed(2)}s`,
    steps,
  };
}

function buildMethodology(patternBeepCount: number): string[] {
  return [
    `Build a ${patternBeepCount}-beep reference waveform in JavaScript and play it once through AudioContext.destination.`,
    'Record only during a short pre-roll and playback window. The recorder is not routed to the speaker.',
    'Subtract the pre-beep noise floor from the microphone capture before correlation and plotting.',
    'Estimate arrival time using schedule offset + baseLatency + outputLatency + inputLatency.',
    'Cross-correlate amplitude envelopes, then compare both the best-match shift and the system-reported shift against the reference.',
  ];
}

function clampRatio(value: number): number {
  return Math.min(1, Math.max(0, value));
}

function buildAnalysisPlots(params: {
  referencePattern: BeepPattern;
  processedRecorded: Float32Array;
  sampleRate: number;
  expectedLagSeconds: number;
  alignment: ReturnType<typeof findPatternAlignment>;
}): Partial<LoopbackAnalysis> {
  const {
    referencePattern,
    processedRecorded,
    sampleRate,
    expectedLagSeconds,
    alignment,
  } = params;

  const patternWindowSeconds =
    referencePattern.durationSeconds + PATTERN_WINDOW_PADDING_SECONDS * 2;
  const halfWindowSamples = Math.floor((patternWindowSeconds * sampleRate) / 2);
  const expectedLagSamples = Math.round(expectedLagSeconds * sampleRate);
  const centerLagSamples = alignment?.lagSamples ?? expectedLagSamples;
  const recordedWindow = buildRecordedWindow(
    processedRecorded,
    centerLagSamples,
    patternWindowSeconds,
    sampleRate
  );
  const expectedWindow = buildRecordedWindow(
    processedRecorded,
    expectedLagSamples,
    patternWindowSeconds,
    sampleRate
  );
  const markerRatio = (lagSamples: number, windowLength: number) =>
    windowLength > 0
      ? clampRatio(
          (lagSamples - Math.max(0, lagSamples - halfWindowSamples)) /
            windowLength
        )
      : null;
  const referenceOverlay = envelopeSeries(
    'Reference',
    colors.blue,
    referencePattern.samples
  );
  const shiftedRecording = (lagSamples: number) =>
    processedRecorded.slice(
      lagSamples,
      lagSamples + referencePattern.samples.length
    );

  return {
    recordedWindowPlot: envelopeSeries(
      'Mic envelope (denoised)',
      colors.yellow,
      recordedWindow
    ),
    overlayPlots: alignment
      ? [
          referenceOverlay,
          envelopeSeries(
            'Matched mic shift',
            colors.accent,
            shiftedRecording(alignment.lagSamples)
          ),
        ]
      : [],
    systemShiftOverlayPlots: [
      referenceOverlay,
      envelopeSeries(
        'System-latency shift',
        colors.green,
        shiftedRecording(expectedLagSamples)
      ),
    ],
    correlationProfile: alignment?.correlationProfile ?? [],
    bestAlignmentMarkerRatio: alignment
      ? markerRatio(centerLagSamples, recordedWindow.length)
      : null,
    systemShiftMarkerRatio: markerRatio(
      expectedLagSamples,
      expectedWindow.length
    ),
    recordedDurationSeconds: recordedWindow.length / sampleRate,
  };
}

function buildAnalysis(
  partial: Partial<LoopbackAnalysis> &
    Pick<LoopbackAnalysis, 'scenario' | 'referencePlot' | 'methodology'>
): LoopbackAnalysis {
  return {
    snapshot: { base: 0, output: 0, input: 0 },
    scheduleOffsetSeconds: 0,
    reportedRoundTripLatency: 0,
    expectedLatencySeconds: 0,
    measuredLatencySeconds: null,
    correlation: null,
    deltaMs: null,
    referenceDurationSeconds: 0,
    recordedDurationSeconds: 0,
    recordedWindowPlot: envelopeSeries(
      'Mic envelope (denoised)',
      colors.yellow,
      []
    ),
    overlayPlots: [],
    systemShiftOverlayPlots: [],
    correlationProfile: [],
    bestAlignmentMarkerRatio: null,
    systemShiftMarkerRatio: null,
    noiseFloor: null,
    ...partial,
  };
}

/**
 * Activates a raw measurement session, plays a beep pattern through the speaker
 * while recording the microphone, and aligns both. The caller owns `io` and
 * releases it with {@link closeLoopbackAudioIO}.
 */
export async function runLoopbackLatencyTest(
  io: LoopbackAudioIO
): Promise<LoopbackAnalysis> {
  const startedAt = Date.now();
  const steps: StepItem[] = [];
  const { audioContext, audioRecorder } = io;
  const { sampleRate } = audioContext;
  const referencePattern = buildBeepReferencePattern(sampleRate);
  const finish = (partial: Partial<LoopbackAnalysis> = {}) =>
    buildAnalysis({
      scenario: createScenario(startedAt, steps),
      referencePlot: envelopeSeries(
        'Reference envelope',
        colors.blue,
        referencePattern.samples
      ),
      referenceDurationSeconds: referencePattern.durationSeconds,
      methodology: buildMethodology(referencePattern.beepCount),
      ...partial,
    });

  AudioManager.setSystemOptions({
    ios: {
      category: 'playAndRecord',
      // 'measurement' keeps the signal path raw (no AGC/EQ) so the loopback
      // delay is accurate, while running the mic+speaker for the shortest
      // possible window to keep the idle full-duplex buzz to a minimum.
      mode: 'measurement',
      options: ['defaultToSpeaker'],
    },
  });
  await AudioManager.setAudioSessionActivity(true);

  if (audioContext.state === 'suspended') {
    await audioContext.resume();
  }

  const recordingStartContextTime = audioContext.currentTime;
  try {
    await audioRecorder.start();
  } catch (error) {
    steps.push({
      id: 'loopback-recorder-start',
      message: 'Recorder is available for loopback measurement',
      status: 'fail',
      details: error instanceof Error ? error.message : String(error),
    });
    return finish();
  }

  const recordedSamples: number[] = [];
  const callbackResult = audioRecorder.onAudioReady(
    { sampleRate, bufferLength: 1024, channelCount: 1 },
    (event) => {
      if (event.buffer) {
        appendChannelSamples(recordedSamples, event.buffer);
      }
    }
  );

  if (callbackResult.status === 'error') {
    steps.push({
      id: 'loopback-callback',
      message: 'Recorder callback is available for loopback measurement',
      status: 'fail',
      details: callbackResult.message,
    });
    return finish();
  }

  await sleep(RECORDER_SETTLE_MS);

  const snapshot = {
    base: audioContext.baseLatency,
    output: audioContext.outputLatency,
    input: audioRecorder.inputLatency,
  };
  const playTime = audioContext.currentTime + PLAY_LEAD_TIME_S;
  const scheduleOffsetSeconds = playTime - recordingStartContextTime;
  const reportedRoundTripLatency =
    snapshot.base + snapshot.output + snapshot.input;
  const expectedLatencySeconds =
    scheduleOffsetSeconds + reportedRoundTripLatency;

  const referenceBuffer = audioContext.createBuffer(
    1,
    referencePattern.samples.length,
    sampleRate
  );
  referenceBuffer.getChannelData(0).set(referencePattern.samples);

  const source = audioContext.createBufferSource();
  source.buffer = referenceBuffer;
  source.connect(audioContext.destination);
  source.start(playTime);
  source.stop(playTime + referencePattern.durationSeconds);

  // Tail must still be long enough to capture the last beep after the full
  // round-trip latency, but no longer, to avoid extra idle buzz. 0.2 s covers
  // realistic loopback delays (schedule + base + output + input).
  const listenWindowSeconds =
    PLAY_LEAD_TIME_S + referencePattern.durationSeconds + 0.2;
  await sleep(listenWindowSeconds * 1000);

  await stopLoopbackAudioIO(io);

  const { processed, noiseFloor } = prepareRecordedSamples(
    recordedSamples,
    sampleRate
  );
  const alignment = findPatternAlignment(
    referencePattern.samples,
    processed,
    sampleRate,
    expectedLatencySeconds
  );
  const measurement = {
    snapshot,
    scheduleOffsetSeconds,
    reportedRoundTripLatency,
    expectedLatencySeconds,
    noiseFloor,
    ...buildAnalysisPlots({
      referencePattern,
      processedRecorded: processed,
      sampleRate,
      expectedLagSeconds: expectedLatencySeconds,
      alignment,
    }),
  };

  if (!alignment) {
    steps.push({
      id: 'loopback-pattern-detected',
      message: 'Recorded waveform matches the generated beep pattern',
      status: 'skipped',
      details: `Captured ${recordedSamples.length} samples (${formatMs(recordedSamples.length / sampleRate)}) with noiseFloor=${noiseFloor.toFixed(4)} but could not align the ${referencePattern.beepCount}-beep reference.`,
    });
    return finish(measurement);
  }

  const measuredLatencySeconds = alignment.lagSeconds;
  const deltaMs = (measuredLatencySeconds - expectedLatencySeconds) * 1000;
  const isCorrelated = alignment.correlation >= MIN_CORRELATION;
  const pass = isCorrelated && Math.abs(deltaMs) <= LOOPBACK_TOLERANCE_MS;

  steps.push({
    id: 'loopback-pattern-detected',
    message: 'Recorded waveform matches the generated beep pattern',
    status: isCorrelated ? 'pass' : 'fail',
    details: `correlation=${alignment.correlation.toFixed(3)}, coarse=${alignment.coarseCorrelation.toFixed(3)}, alignedAt=${formatMs(measuredLatencySeconds)}, noiseFloor=${noiseFloor.toFixed(4)}`,
  });
  steps.push({
    id: 'loopback-latency-match',
    message:
      'Aligned loopback delay matches baseLatency + outputLatency + inputLatency',
    status: pass ? 'pass' : 'fail',
    details: `measured=${formatMs(measuredLatencySeconds)}, expected=${formatMs(expectedLatencySeconds)} (schedule ${formatMs(scheduleOffsetSeconds)} + base ${formatMs(snapshot.base)} + output ${formatMs(snapshot.output)} + input ${formatMs(snapshot.input)}), delta=${deltaMs.toFixed(2)} ms`,
  });

  return finish({
    ...measurement,
    measuredLatencySeconds,
    correlation: alignment.correlation,
    deltaMs,
  });
}
