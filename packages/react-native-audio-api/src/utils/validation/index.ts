export { validateAudioNodeOptions } from './audioNodeOptions';

export {
  ANALYSER_ALLOWED_FFT_SIZE,
  AnalyserOptionsValidator,
  validateAnalyserFftSize,
  validateAnalyserMaxDecibels,
  validateAnalyserMinDecibels,
  validateAnalyserSmoothingTimeConstant,
} from './analyser';

export {
  ConvolverOptionsValidator,
  validateConvolverBufferChannelCount,
  validateConvolverBufferSampleRate,
} from './convolver';

export { OscillatorOptionsValidator } from './oscillator';

export { PeriodicWaveOptionsValidator } from './periodicWave';

export { validateWaveShaperCurve } from './waveShaper';

export {
  assertSupportedSampleRate,
  MAX_SUPPORTED_SAMPLE_RATE,
  MIN_SUPPORTED_SAMPLE_RATE,
} from './sampleRate';

export { validateIIRFilterOptions } from './iirFilter';

export {
  assertSupportedDecodeStringSource,
  assertUnsupportedDecodeStringFormats,
} from './decodeSource';
