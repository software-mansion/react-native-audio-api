'use strict';

/**
 * Slim Web Audio API surface for the Node WPT harness.
 * Only spec-defined classes are exported — no RN extensions
 * (recorder, decoder, worklet nodes, etc.).
 */

const path = require('node:path');

const packageRoot = path.resolve(__dirname, '..');

function loadDefault(relativePath) {
  const mod = require(path.join(packageRoot, relativePath));
  return mod.default ?? mod;
}

const WEB_AUDIO_CLASSES = [
  'AnalyserNode',
  'AudioBuffer',
  'AudioBufferSourceNode',
  'AudioContext',
  'AudioDestinationNode',
  'AudioNode',
  'AudioParam',
  'AudioScheduledSourceNode',
  'BaseAudioContext',
  'BiquadFilterNode',
  'ConstantSourceNode',
  'ConvolverNode',
  'DelayNode',
  'GainNode',
  'IIRFilterNode',
  'OfflineAudioContext',
  'OscillatorNode',
  'PeriodicWave',
  'StereoPannerNode',
  'PannerNode',
  'WaveShaperNode',
];

const api = {};

for (const name of WEB_AUDIO_CLASSES) {
  try {
    api[name] = loadDefault(`lib/commonjs/core/${name}`);
  } catch (error) {
    throw new Error(
      `Failed to load ${name} for WPT (${error.message}). Run yarn build first.`,
      { cause: error }
    );
  }
}

const errors = require(path.join(packageRoot, 'lib/commonjs/errors'));
Object.assign(api, {
  AudioApiError: errors.AudioApiError,
  IndexSizeError: errors.IndexSizeError,
  InvalidAccessError: errors.InvalidAccessError,
  InvalidStateError: errors.InvalidStateError,
  NotSupportedError: errors.NotSupportedError,
});

module.exports = api;
