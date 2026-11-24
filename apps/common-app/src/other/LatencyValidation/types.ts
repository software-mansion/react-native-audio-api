import type { ScenarioItem } from '../../testComponents';

export interface LatencySnapshot {
  base: number;
  output: number;
  input: number;
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
  scenario: ScenarioItem;
  snapshot: LatencySnapshot;
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
