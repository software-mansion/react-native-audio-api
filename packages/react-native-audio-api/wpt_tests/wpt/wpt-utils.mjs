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

function wrapWithRealmErrors(window, fn) {
  return function (...args) {
    try {
      const result = fn.apply(this, args);
      if (isThenable(result)) {
        return toWindowRealmPromise(window, result);
      }
      return result;
    } catch (error) {
      throw toWindowRealmError(window, error);
    }
  };
}

function wrapPrototypeMembers(window, ctor) {
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

    if (desc.get != null || desc.set != null) {
      const replacement = { ...desc };
      if (desc.get != null) {
        replacement.get = wrapWithRealmErrors(window, desc.get);
      }
      if (desc.set != null) {
        replacement.set = wrapWithRealmErrors(window, desc.set);
      }
      Object.defineProperty(proto, key, replacement);
      continue;
    }

    if (typeof desc.value === 'function') {
      Object.defineProperty(proto, key, {
        ...desc,
        value: wrapWithRealmErrors(window, desc.value),
      });
    }
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
    wrapPrototypeMembers(window, window[name]);

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

  const originalCopyFromChannel = AudioBuffer.prototype.copyFromChannel;
  const originalCopyToChannel = AudioBuffer.prototype.copyToChannel;

  AudioBuffer.prototype.copyFromChannel = function copyFromChannel(
    destination,
    channelNumber,
    startInChannel = 0
  ) {
    assertFloat32Array(destination, 'destination', window);
    return originalCopyFromChannel.call(
      this,
      destination,
      channelNumber,
      startInChannel
    );
  };

  AudioBuffer.prototype.copyToChannel = function copyToChannel(
    source,
    channelNumber,
    startInChannel = 0
  ) {
    assertFloat32Array(source, 'source', window);
    return originalCopyToChannel.call(this, source, channelNumber, startInChannel);
  };
}
