export interface LatencySnapshot {
  base: number;
  output: number;
  input: number;
  sampleRate: number;
  contextState: string;
  recorderActive: boolean;
}

export type LatencyTestStatus = 'pass' | 'fail' | 'info' | 'skipped';

export interface LatencyTestStep {
  id: string;
  message: string;
  status: LatencyTestStatus;
  details?: string;
}

export interface LatencyTestScenario {
  id: string;
  title: string;
  status: 'pass' | 'fail' | 'skipped';
  durationMs: number;
  steps: LatencyTestStep[];
}

export interface WaveformSeries {
  label: string;
  color: string;
  points: number[];
}

export interface CorrelationPoint {
  lagMs: number;
  score: number;
}

export interface LoopbackAnalysis {
  scenario: LatencyTestScenario;
  snapshot: LatencySnapshot;
  recordingStartContextTime: number;
  playTime: number;
  scheduleOffsetSeconds: number;
  reportedRoundTripLatency: number;
  expectedLatencySeconds: number;
  measuredLatencySeconds: number | null;
  correlation: number | null;
  deltaMs: number | null;
  referenceDurationSeconds: number;
  recordedDurationSeconds: number;
  referencePlot: WaveformSeries;
  recordedWindowPlot: WaveformSeries;
  overlayPlots: WaveformSeries[];
  systemShiftOverlayPlots: WaveformSeries[];
  correlationProfile: CorrelationPoint[];
  bestAlignmentMarkerRatio: number | null;
  systemShiftMarkerRatio: number | null;
  noiseFloor: number | null;
  methodology: string[];
}
