export type TimeStretchingAlgorithm = 'linear' | 'pitchCorrection';

export interface AudioSettings {
  pitchCorrection: boolean;
}

export const TIME_STRETCHING_OPTIONS: TimeStretchingAlgorithm[] = [
  'linear',
  'pitchCorrection',
] as const;
