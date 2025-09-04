export type TimeStretchingAlgorithm = 'linear' | 'pitchCorrection' | 'PSOLA';

export interface AudioSettings {
  pitchCorrection: boolean;
  PSOLA: boolean;
}

export const TIME_STRETCHING_OPTIONS: TimeStretchingAlgorithm[] = [
  'linear',
  'pitchCorrection',
  'PSOLA',
] as const;
