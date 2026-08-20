/**
 * Wrap Web Audio node constructors so invalid-argument TypeErrors are thrown
 * from the jsdom window realm. Node-loaded modules otherwise throw Node's
 * TypeError (e.g. on `undefined.context`), which audit.js rejects.
 */

import {
  getCurrentTestWindow,
  patchPrototypeOnce,
  toWindowRealmError,
  WRAPPED_NODE_CONSTRUCTORS,
} from './wpt-utils.mjs';

const AUDIO_NODE_CONSTRUCTORS = [
  { name: 'AnalyserNode' },
  { name: 'AudioBufferSourceNode' },
  { name: 'BiquadFilterNode' },
  { name: 'ChannelMergerNode' },
  { name: 'ChannelSplitterNode' },
  { name: 'ConstantSourceNode' },
  { name: 'ConvolverNode' },
  { name: 'DelayNode' },
  { name: 'GainNode' },
  { name: 'IIRFilterNode', optionsRequired: true },
  { name: 'OscillatorNode' },
  { name: 'StereoPannerNode' },
  { name: 'WaveShaperNode' },
];

function isBaseAudioContext(value) {
  return (
    value != null &&
    typeof value === 'object' &&
    'context' in value &&
    'destination' in value &&
    'sampleRate' in value
  );
}

function isOptionsDictionary(value) {
  if (value === undefined || value === null) {
    return true;
  }

  return typeof value === 'object' && !Array.isArray(value);
}

function wrapAudioNodeConstructor(Original, window, optionsRequired = false) {
  function Wrapped(...args) {
    const [context, options] = args;

    if (!isBaseAudioContext(context)) {
      throw new window.TypeError();
    }

    if (optionsRequired && (options === undefined || options === null)) {
      throw new window.TypeError();
    }

    if (!isOptionsDictionary(options)) {
      throw new window.TypeError();
    }

    try {
      return Reflect.construct(Original, args, new.target ?? Wrapped);
    } catch (error) {
      throw toWindowRealmError(window, error);
    }
  }

  Wrapped.prototype = Original.prototype;
  Object.defineProperty(Wrapped, 'name', { value: Original.name });

  return Wrapped;
}

export function wrapAudioNodeConstructors(window) {
  for (const { name, optionsRequired = false } of AUDIO_NODE_CONSTRUCTORS) {
    const Original = window[name];
    if (typeof Original !== 'function') {
      continue;
    }

    window[name] = wrapAudioNodeConstructor(Original, window, optionsRequired);
    WRAPPED_NODE_CONSTRUCTORS.add(name);
  }

  wrapAudioNodeConnectDisconnect(window);
}

function wrapAudioNodeConnectDisconnect(window) {
  const AudioNode = window.AudioNode;
  if (typeof AudioNode !== 'function') {
    return;
  }

  // AudioNode is shared by every test window, so these go on once and resolve the
  // window at call time — see patchPrototypeOnce.
  for (const methodName of ['connect', 'disconnect']) {
    patchPrototypeOnce(
      AudioNode.prototype,
      methodName,
      (original) =>
        function (...args) {
          try {
            return original.apply(this, args);
          } catch (error) {
            throw toWindowRealmError(getCurrentTestWindow(), error);
          }
        },
      'connect-realm-errors'
    );
  }
}
