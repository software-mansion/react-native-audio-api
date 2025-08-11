import { TimeStretchingAlgorithm, AudioSettings } from './types';

export const getAudioSettings = (
  algorithm: TimeStretchingAlgorithm
): AudioSettings => {
  switch (algorithm) {
    case 'linear':
      return { pitchCorrection: false, PSOLA: false };
    case 'pitchCorrection':
      return { pitchCorrection: true, PSOLA: false };
    case 'PSOLA':
      return { pitchCorrection: false, PSOLA: true };
  }
};
