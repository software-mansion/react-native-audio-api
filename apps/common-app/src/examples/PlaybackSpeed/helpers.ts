import { TimeStretchingAlgorithm, AudioSettings } from './types';

export const getAudioSettings = (
  algorithm: TimeStretchingAlgorithm
): AudioSettings => {
  switch (algorithm) {
    case 'linear':
      return { pitchCorrection: false };
    case 'pitchCorrection':
      return { pitchCorrection: true };
  }
};
