/**
 * Wrap Web Audio node constructors so invalid-argument TypeErrors are thrown
 * from the jsdom window realm. Node-loaded modules otherwise throw Node's
 * TypeError (e.g. on `undefined.context`), which audit.js rejects.
 */

const AUDIO_NODE_CONSTRUCTORS = [
  { name: 'AnalyserNode' },
  { name: 'AudioBufferSourceNode' },
  { name: 'BiquadFilterNode' },
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

    return Reflect.construct(Original, args, new.target ?? Wrapped);
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
  }
}
