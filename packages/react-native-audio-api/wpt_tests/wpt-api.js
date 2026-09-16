'use strict';

/**
 * Slim Web Audio API surface for the Node WPT harness.
 * Only spec-defined classes are exported — no RN extensions
 * (recorder, decoder, worklet nodes, etc.).
 */

const path = require('node:path');

const packageRoot = require('./package-root');

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
  'ChannelMergerNode',
  'ChannelSplitterNode',
  'ConstantSourceNode',
  'ConvolverNode',
  'DelayNode',
  'GainNode',
  'IIRFilterNode',
  'OfflineAudioContext',
  'OscillatorNode',
  'PeriodicWave',
  'StereoPannerNode',
  'WaveShaperNode',
];

const api = {};

for (const name of WEB_AUDIO_CLASSES) {
  try {
    api[name] = loadDefault(`lib/commonjs/core/${name}`);
  } catch (error) {
    if (process.env.RN_AUDIO_API_APP_ROOT) {
      console.warn(
        `[wpt] ${name} is not available in ${packageRoot} — tests using it will fail.`
      );
      continue;
    }
    throw new Error(
      `Failed to load ${name} for WPT (${error.message}). Run yarn build first.`,
      { cause: error }
    );
  }
}

const errors = require(path.join(packageRoot, 'lib/commonjs/errors'));
const ERROR_CLASSES = [
  'AudioApiError',
  'IndexSizeError',
  'InvalidAccessError',
  'InvalidStateError',
  'NotSupportedError',
];
for (const name of ERROR_CLASSES) {
  if (errors[name] != null) {
    api[name] = errors[name];
  }
}

module.exports = api;
