import AudioBuffer from './AudioBuffer.web';

export default interface ConvolverNodeOptions {
  buffer?: AudioBuffer | null;
  disableNormalization?: boolean;
}
