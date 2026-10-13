import {
  AudioManager,
  type AudioContext,
  type AudioRecorder,
} from 'react-native-audio-api';
import { Platform } from 'react-native';

import {
  appendChannelSamples,
  buildBeepReferencePattern,
  buildRecordedWindow,
  downsampleEnvelopeForPlot,
  findPatternAlignment,
  formatMs,
  prepareRecordedSamples,
  sleep,
} from './helpers';
import type {
  LatencySnapshot,
  LatencyTestScenario,
  LatencyTestStep,
  LoopbackAnalysis,
  WaveformSeries,
} from './types';

// Keep the live full-duplex window (mic + speaker both active) as short as
// possible: the raw loopback path produces an idle buzz, so we only settle the
// recorder briefly and use a short scheduling lead before the beep plays.
const RECORDER_SETTLE_MS = 70;
const PLAY_LEAD_TIME_S = 0.07;
const LOOPBACK_TOLERANCE_MS = 120;
const MIN_CORRELATION = 0.45;
const PATTERN_WINDOW_PADDING_SECONDS = 0.12;

interface LatencyTestContext {
  audioContext: AudioContext;
  audioRecorder: AudioRecorder;
}

function readSnapshot(
  audioContext: AudioContext,
  audioRecorder: AudioRecorder
): LatencySnapshot {
  return {
    base: audioContext.baseLatency,
    output: audioContext.outputLatency,
    input: audioRecorder.inputLatency,
    sampleRate: audioContext.sampleRate,
    contextState: audioContext.state,
    recorderActive: audioRecorder.isRecording(),
  };
}

function createScenario(
  id: string,
  title: string,
  startedAt: number,
  steps: LatencyTestStep[]
): LatencyTestScenario {
  const status = steps.some((step) => step.status === 'fail')
    ? 'fail'
    : steps.every((step) => step.status === 'skipped')
      ? 'skipped'
      : 'pass';

  return {
    id,
    title,
    status,
    durationMs: Date.now() - startedAt,
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

function buildEmptyPlots(): Pick<
  LoopbackAnalysis,
  | 'referencePlot'
  | 'recordedWindowPlot'
  | 'overlayPlots'
  | 'systemShiftOverlayPlots'
  | 'correlationProfile'
  | 'bestAlignmentMarkerRatio'
  | 'systemShiftMarkerRatio'
> {
  return {
    referencePlot: { label: 'Reference envelope', color: '#38ACDD', points: [] },
    recordedWindowPlot: {
      label: 'Mic envelope (denoised)',
      color: '#FFD61E',
      points: [],
    },
    overlayPlots: [],
    systemShiftOverlayPlots: [],
    correlationProfile: [],
    bestAlignmentMarkerRatio: null,
    systemShiftMarkerRatio: null,
  };
}

function buildAnalysisPlots(params: {
  referencePattern: ReturnType<typeof buildBeepReferencePattern>;
  processedRecorded: Float32Array;
  sampleRate: number;
  expectedLagSeconds: number;
  alignment: ReturnType<typeof findPatternAlignment>;
}): Pick<
  LoopbackAnalysis,
  | 'referencePlot'
  | 'recordedWindowPlot'
  | 'overlayPlots'
  | 'systemShiftOverlayPlots'
  | 'correlationProfile'
  | 'bestAlignmentMarkerRatio'
  | 'systemShiftMarkerRatio'
  | 'recordedDurationSeconds'
> {
  const {
    referencePattern,
    processedRecorded,
    sampleRate,
    expectedLagSeconds,
    alignment,
  } = params;

  const patternWindowSeconds =
    referencePattern.durationSeconds + PATTERN_WINDOW_PADDING_SECONDS * 2;
  const centerLagSamples = alignment?.lagSamples ??
    Math.round(expectedLagSeconds * sampleRate);
  const expectedLagSamples = Math.round(expectedLagSeconds * sampleRate);
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
  const windowStartSample = Math.max(
    0,
    centerLagSamples - Math.floor((patternWindowSeconds * sampleRate) / 2)
  );
  const expectedWindowStartSample = Math.max(
    0,
    expectedLagSamples - Math.floor((patternWindowSeconds * sampleRate) / 2)
  );

  const referencePlot: WaveformSeries = {
    label: 'Reference envelope',
    color: '#38ACDD',
    points: downsampleEnvelopeForPlot(referencePattern.samples),
  };

  const recordedWindowPlot: WaveformSeries = {
    label: 'Mic envelope (denoised)',
    color: '#FFD61E',
    points: downsampleEnvelopeForPlot(recordedWindow),
  };

  const overlayPlots: WaveformSeries[] =
    alignment === null
      ? []
      : [
          {
            label: 'Reference',
            color: '#38ACDD',
            points: downsampleEnvelopeForPlot(referencePattern.samples),
          },
          {
            label: 'Matched mic shift',
            color: '#FF7A7A',
            points: downsampleEnvelopeForPlot(
              processedRecorded.slice(
                alignment.lagSamples,
                alignment.lagSamples + referencePattern.samples.length
              )
            ),
          },
        ];

  const systemShiftOverlayPlots: WaveformSeries[] = [
    {
      label: 'Reference',
      color: '#38ACDD',
      points: downsampleEnvelopeForPlot(referencePattern.samples),
    },
    {
      label: 'System-latency shift',
      color: '#9BE564',
      points: downsampleEnvelopeForPlot(
        processedRecorded.slice(
          expectedLagSamples,
          expectedLagSamples + referencePattern.samples.length
        )
      ),
    },
  ];

  return {
    referencePlot,
    recordedWindowPlot,
    overlayPlots,
    systemShiftOverlayPlots,
    correlationProfile: alignment?.correlationProfile ?? [],
    bestAlignmentMarkerRatio:
      recordedWindow.length > 0
        ? Math.min(
            1,
            Math.max(0, (centerLagSamples - windowStartSample) / recordedWindow.length)
          )
        : null,
    systemShiftMarkerRatio:
      expectedWindow.length > 0
        ? Math.min(
            1,
            Math.max(
              0,
              (expectedLagSamples - expectedWindowStartSample) / expectedWindow.length
            )
          )
        : null,
    recordedDurationSeconds: recordedWindow.length / sampleRate,
  };
}

function createEmptyAnalysis(
  scenario: LatencyTestScenario,
  partial: Partial<LoopbackAnalysis>
): LoopbackAnalysis {
  const emptyPlots = buildEmptyPlots();

  return {
    scenario,
    snapshot: partial.snapshot ?? {
      base: 0,
      output: 0,
      input: 0,
      sampleRate: 44100,
      contextState: 'suspended',
      recorderActive: false,
    },
    recordingStartContextTime: partial.recordingStartContextTime ?? 0,
    playTime: partial.playTime ?? 0,
    scheduleOffsetSeconds: partial.scheduleOffsetSeconds ?? 0,
    reportedRoundTripLatency: partial.reportedRoundTripLatency ?? 0,
    expectedLatencySeconds: partial.expectedLatencySeconds ?? 0,
    measuredLatencySeconds: partial.measuredLatencySeconds ?? null,
    correlation: partial.correlation ?? null,
    deltaMs: partial.deltaMs ?? null,
    referenceDurationSeconds: partial.referenceDurationSeconds ?? 0,
    recordedDurationSeconds: partial.recordedDurationSeconds ?? 0,
    noiseFloor: partial.noiseFloor ?? null,
    methodology: partial.methodology ?? buildMethodology(5),
    ...emptyPlots,
    ...partial,
  };
}

async function runLoopbackLatencyTest(
  context: LatencyTestContext
): Promise<LoopbackAnalysis> {
  const startedAt = Date.now();
  const steps: LatencyTestStep[] = [];
  const { audioContext, audioRecorder } = context;

  await stopLoopbackAudioIO(context);
  await sleep(100);

  if (audioContext.state === 'suspended') {
    await audioContext.resume();
  }

  const recordingStartContextTime = audioContext.currentTime;
  const startResult = await audioRecorder.start();
  if (startResult.status !== 'success') {
    steps.push({
      id: 'loopback-recorder-start',
      message: 'Recorder is available for loopback measurement',
      status: 'fail',
      details: startResult.message,
    });

    return createEmptyAnalysis(
      createScenario(
        'loopback-latency',
        'Speaker-to-mic loopback check',
        startedAt,
        steps
      ),
      { recordingStartContextTime }
    );
  }

  const referencePattern = buildBeepReferencePattern(audioContext.sampleRate);
  const recordedSamples: number[] = [];

  const callbackResult = audioRecorder.onAudioReady(
    {
      sampleRate: audioContext.sampleRate,
      bufferLength: 1024,
      channelCount: 1,
    },
    (event) => {
      if (!event.buffer) {
        return;
      }

      appendChannelSamples(recordedSamples, event.buffer);
    }
  );

  if (callbackResult.status === 'error') {
    steps.push({
      id: 'loopback-callback',
      message: 'Recorder callback is available for loopback measurement',
      status: 'fail',
      details: callbackResult.message,
    });

    return createEmptyAnalysis(
      createScenario(
        'loopback-latency',
        'Speaker-to-mic loopback check',
        startedAt,
        steps
      ),
      {
        recordingStartContextTime,
        referenceDurationSeconds: referencePattern.durationSeconds,
        referencePlot: {
          label: 'Reference envelope',
          color: '#38ACDD',
          points: downsampleEnvelopeForPlot(referencePattern.samples),
        },
        methodology: buildMethodology(referencePattern.beepCount),
      }
    );
  }

  await sleep(RECORDER_SETTLE_MS);

  const snapshot = readSnapshot(audioContext, audioRecorder);
  const playTime = audioContext.currentTime + PLAY_LEAD_TIME_S;
  const scheduleOffsetSeconds = playTime - recordingStartContextTime;
  const reportedRoundTripLatency =
    snapshot.base + snapshot.output + snapshot.input;
  const expectedLatencySeconds =
    scheduleOffsetSeconds + reportedRoundTripLatency;

  const referenceBuffer = audioContext.createBuffer(
    1,
    referencePattern.samples.length,
    audioContext.sampleRate
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

  audioRecorder.clearOnAudioReady();
  await stopLoopbackAudioIO(context);

  const { processed, noiseFloor } = prepareRecordedSamples(
    recordedSamples,
    audioContext.sampleRate
  );

  const alignment = findPatternAlignment(
    referencePattern.samples,
    processed,
    audioContext.sampleRate,
    expectedLatencySeconds
  );

  const plotData = buildAnalysisPlots({
    referencePattern,
    processedRecorded: processed,
    sampleRate: audioContext.sampleRate,
    expectedLagSeconds: expectedLatencySeconds,
    alignment,
  });

  if (!alignment) {
    steps.push({
      id: 'loopback-pattern-detected',
      message: 'Recorded waveform matches the generated beep pattern',
      status: 'skipped',
      details: `Captured ${recordedSamples.length} samples (${formatMs(recordedSamples.length / audioContext.sampleRate)}) with noiseFloor=${noiseFloor.toFixed(4)} but could not align the ${referencePattern.beepCount}-beep reference.`,
    });

    return createEmptyAnalysis(
      createScenario(
        'loopback-latency',
        'Speaker-to-mic loopback check',
        startedAt,
        steps
      ),
      {
        snapshot,
        recordingStartContextTime,
        playTime,
        scheduleOffsetSeconds,
        reportedRoundTripLatency,
        expectedLatencySeconds,
        referenceDurationSeconds: referencePattern.durationSeconds,
        recordedDurationSeconds: plotData.recordedDurationSeconds,
        noiseFloor,
        methodology: buildMethodology(referencePattern.beepCount),
        referencePlot: plotData.referencePlot,
        recordedWindowPlot: plotData.recordedWindowPlot,
        systemShiftOverlayPlots: plotData.systemShiftOverlayPlots,
        correlationProfile: plotData.correlationProfile,
        systemShiftMarkerRatio: plotData.systemShiftMarkerRatio,
      }
    );
  }

  const measuredLatencySeconds = alignment.lagSeconds;
  const deltaMs = (measuredLatencySeconds - expectedLatencySeconds) * 1000;
  const pass =
    alignment.correlation >= MIN_CORRELATION &&
    measuredLatencySeconds >=
      expectedLatencySeconds - LOOPBACK_TOLERANCE_MS / 1000 &&
    measuredLatencySeconds <=
      expectedLatencySeconds + LOOPBACK_TOLERANCE_MS / 1000;

  steps.push({
    id: 'loopback-pattern-detected',
    message: 'Recorded waveform matches the generated beep pattern',
    status: alignment.correlation >= MIN_CORRELATION ? 'pass' : 'fail',
    details: `correlation=${alignment.correlation.toFixed(3)}, coarse=${alignment.coarseCorrelation.toFixed(3)}, alignedAt=${formatMs(measuredLatencySeconds)}, noiseFloor=${noiseFloor.toFixed(4)}`,
  });

  steps.push({
    id: 'loopback-latency-match',
    message:
      'Aligned loopback delay matches baseLatency + outputLatency + inputLatency',
    status: pass ? 'pass' : 'fail',
    details: `measured=${formatMs(measuredLatencySeconds)}, expected=${formatMs(expectedLatencySeconds)} (schedule ${formatMs(scheduleOffsetSeconds)} + base ${formatMs(snapshot.base)} + output ${formatMs(snapshot.output)} + input ${formatMs(snapshot.input)}), delta=${deltaMs.toFixed(2)} ms`,
  });

  return createEmptyAnalysis(
    createScenario(
      'loopback-latency',
      'Speaker-to-mic loopback check',
      startedAt,
      steps
    ),
    {
      snapshot,
      recordingStartContextTime,
      playTime,
      scheduleOffsetSeconds,
      reportedRoundTripLatency,
      expectedLatencySeconds,
      measuredLatencySeconds,
      correlation: alignment.correlation,
      deltaMs,
      referenceDurationSeconds: referencePattern.durationSeconds,
      recordedDurationSeconds: plotData.recordedDurationSeconds,
      noiseFloor,
      methodology: buildMethodology(referencePattern.beepCount),
      referencePlot: plotData.referencePlot,
      recordedWindowPlot: plotData.recordedWindowPlot,
      overlayPlots: plotData.overlayPlots,
      systemShiftOverlayPlots: plotData.systemShiftOverlayPlots,
      correlationProfile: plotData.correlationProfile,
      bestAlignmentMarkerRatio: plotData.bestAlignmentMarkerRatio,
      systemShiftMarkerRatio: plotData.systemShiftMarkerRatio,
    }
  );
}

export async function runLatencyValidationSuite(
  context: LatencyTestContext
): Promise<LoopbackAnalysis> {
  return runLoopbackLatencyTest(context);
}

export async function stopLoopbackAudioIO(
  context: LatencyTestContext
): Promise<void> {
  const { audioContext, audioRecorder } = context;

  audioRecorder.clearOnAudioReady();
  audioRecorder.disconnect();

  if (audioRecorder.isRecording()) {
    await audioRecorder.stop();
  }

  if (audioContext.state === 'running') {
    await audioContext.suspend();
  }

  if (Platform.OS !== 'web') {
    await AudioManager.setAudioSessionActivity(false);
  }
}

export async function cleanupLatencyValidation(
  context: LatencyTestContext
): Promise<void> {
  await stopLoopbackAudioIO(context);
}
