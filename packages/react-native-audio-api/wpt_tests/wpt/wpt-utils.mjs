/**
 * WPT-only helpers. Not used in React Native production code.
 */

const DOM_EXCEPTION_NAMES = new Set([
  'IndexSizeError',
  'InvalidAccessError',
  'InvalidStateError',
  'NotSupportedError',
  'SyntaxError',
  'TypeError',
  'SecurityError',
  'NetworkError',
  'AbortError',
  'DataError',
  'EncodingError',
  'NotReadableError',
  'UnknownError',
  'ConstraintError',
  'QuotaExceededError',
  'TimeoutError',
]);

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

/**
 * The jsdom window of the test file currently running.
 *
 * The Web Audio classes are loaded once and shared by every test window, so their
 * prototypes must be patched once, not per window. A patch that closed over its
 * `window` would keep that window — and every AudioContext created in it — alive for
 * the rest of the run, so the shared patches read the current window from here instead.
 */
let currentTestWindow = null;

export function setCurrentTestWindow(window) {
  currentTestWindow = window;
}

export function getCurrentTestWindow() {
  return currentTestWindow;
}

/** Prototype -> "layer:member" pairs already patched, so setup() cannot chain wrappers. */
const patchedPrototypeMembers = new WeakMap();

/**
 * Claim `prototype[key]` for one patch layer. Returns false if that layer already
 * claimed it, which is how repeated setup() calls avoid stacking a new wrapper on the
 * previous one.
 *
 * Layers are independent and compose: several of them legitimately wrap the same member
 * (realm errors around the Float32Array assertion, say), and each must land exactly once.
 */
function claimPrototypeMember(prototype, key, layer) {
  let patched = patchedPrototypeMembers.get(prototype);
  if (patched == null) {
    patched = new Set();
    patchedPrototypeMembers.set(prototype, patched);
  }
  const claim = `${layer}:${key}`;
  if (patched.has(claim)) {
    return false;
  }
  patched.add(claim);
  return true;
}

/**
 * Install `wrap(original)` as `prototype[methodName]`, at most once per prototype.
 * Later calls are no-ops, which keeps the wrapper depth at one however many test
 * files run.
 *
 * @param {object} prototype
 * @param {string} methodName
 * @param {(original: Function) => Function} wrap
 * @param {string} layer identifies the patch, so independent layers can each apply once
 */
export function patchPrototypeOnce(prototype, methodName, wrap, layer) {
  if (prototype == null || typeof prototype[methodName] !== 'function') {
    return;
  }

  if (!claimPrototypeMember(prototype, methodName, layer)) {
    return;
  }

  prototype[methodName] = wrap(prototype[methodName]);
}

/** Node constructors already wrapped for invalid-argument TypeErrors. */
export const WRAPPED_NODE_CONSTRUCTORS = new Set([
  'AnalyserNode',
  'AudioBufferSourceNode',
  'BiquadFilterNode',
  'ChannelMergerNode',
  'ChannelSplitterNode',
  'ConstantSourceNode',
  'ConvolverNode',
  'DelayNode',
  'GainNode',
  'IIRFilterNode',
  'OscillatorNode',
  'StereoPannerNode',
  'WaveShaperNode',
]);

/**
 * Re-throw library errors in the jsdom window realm when the name matches a
 * Web IDL exception (DOMException) or built-in (TypeError / RangeError).
 */
export function toWindowRealmError(window, error) {
  if (error == null) {
    return error;
  }

  if (error instanceof window.DOMException) {
    return error;
  }

  const name = error?.name;
  if (
    typeof name === 'string' &&
    DOM_EXCEPTION_NAMES.has(name) &&
    name !== 'TypeError' &&
    window.DOMException != null
  ) {
    return new window.DOMException(error.message, name);
  }

  if (name === 'TypeError' && !(error instanceof window.TypeError)) {
    return new window.TypeError(error.message);
  }

  if (name === 'RangeError' && !(error instanceof window.RangeError)) {
    return new window.RangeError(error.message);
  }

  return error;
}

function isThenable(value) {
  return value != null && typeof value.then === 'function';
}

/**
 * Library classes load in Node's realm; jsdom tests use `window.Promise`.
 * `nodePromise instanceof window.Promise` is false across realms, which breaks
 * WPT checks like `assert_true(p instanceof Promise)`. Re-wrap thenables into
 * the window realm and map rejection reasons to window errors.
 */
function toWindowRealmPromise(window, thenable) {
  return new window.Promise((resolve, reject) => {
    thenable.then(
      (value) => resolve(value),
      (error) => reject(toWindowRealmError(window, error))
    );
  });
}

function wrapWithRealmErrors(fn) {
  return function (...args) {
    try {
      const result = fn.apply(this, args);
      if (isThenable(result)) {
        return toWindowRealmPromise(getCurrentTestWindow(), result);
      }
      return result;
    } catch (error) {
      throw toWindowRealmError(getCurrentTestWindow(), error);
    }
  };
}

/**
 * Wrap every method and accessor on `ctor.prototype` so errors surface in the test's
 * realm. The Web Audio classes are shared by all test windows, so each member is
 * wrapped once and the wrapper looks the window up per call.
 */
function wrapPrototypeMembers(ctor) {
  if (typeof ctor !== 'function') {
    return;
  }

  const proto = ctor.prototype;
  for (const key of Object.getOwnPropertyNames(proto)) {
    if (key === 'constructor') {
      continue;
    }

    const desc = Object.getOwnPropertyDescriptor(proto, key);
    if (desc == null) {
      continue;
    }

    const isAccessor = desc.get != null || desc.set != null;
    if (!isAccessor && typeof desc.value !== 'function') {
      continue;
    }
    if (!claimPrototypeMember(proto, key, 'realm-errors')) {
      continue;
    }

    if (isAccessor) {
      const replacement = { ...desc };
      if (desc.get != null) {
        replacement.get = wrapWithRealmErrors(desc.get);
      }
      if (desc.set != null) {
        replacement.set = wrapWithRealmErrors(desc.set);
      }
      Object.defineProperty(proto, key, replacement);
      continue;
    }

    Object.defineProperty(proto, key, {
      ...desc,
      value: wrapWithRealmErrors(desc.value),
    });
  }
}

function wrapConstructorWithRealmErrors(window, name) {
  const Original = window[name];
  if (typeof Original !== 'function') {
    return;
  }

  function Wrapped(...args) {
    try {
      return Reflect.construct(Original, args, new.target ?? Wrapped);
    } catch (error) {
      throw toWindowRealmError(window, error);
    }
  }

  Wrapped.prototype = Original.prototype;
  Object.defineProperty(Wrapped, 'name', { value: Original.name });
  window[name] = Wrapped;
}

export function wrapWebAudioRealmErrors(window) {
  for (const name of WEB_AUDIO_CLASSES) {
    wrapPrototypeMembers(window[name]);

    if (!WRAPPED_NODE_CONSTRUCTORS.has(name)) {
      wrapConstructorWithRealmErrors(window, name);
    }
  }
}

export function assertFloat32Array(value, name, window) {
  const TypeErrorCtor = window?.TypeError ?? TypeError;

  const isFloat32View =
    value instanceof Float32Array ||
    (typeof value === 'object' &&
      value !== null &&
      ArrayBuffer.isView(value) &&
      value.BYTES_PER_ELEMENT === 4 &&
      value.constructor?.name === 'Float32Array');

  if (!isFloat32View) {
    throw new TypeErrorCtor(`The provided ${name} is not a Float32Array`);
  }

  if (value.buffer?.constructor?.name === 'SharedArrayBuffer') {
    throw new TypeErrorCtor(
      `The provided ${name} is backed by a SharedArrayBuffer, which is not allowed`
    );
  }
}

export function wrapAudioBufferCopyMethods(window) {
  const AudioBuffer = window.AudioBuffer;
  if (typeof AudioBuffer !== 'function') {
    return;
  }

  patchPrototypeOnce(
    AudioBuffer.prototype,
    'copyFromChannel',
    (original) =>
      function copyFromChannel(destination, channelNumber, startInChannel = 0) {
        assertFloat32Array(destination, 'destination', getCurrentTestWindow());
        return original.call(this, destination, channelNumber, startInChannel);
      },
    'float32-assert'
  );

  patchPrototypeOnce(
    AudioBuffer.prototype,
    'copyToChannel',
    (original) =>
      function copyToChannel(source, channelNumber, startInChannel = 0) {
        assertFloat32Array(source, 'source', getCurrentTestWindow());
        return original.call(this, source, channelNumber, startInChannel);
      },
    'float32-assert'
  );
}
