import AudioBuffer from './AudioBuffer';

export interface ConvolverNodeOptions {
  buffer?: AudioBuffer | null;
  disableNormalization?: boolean;
}
