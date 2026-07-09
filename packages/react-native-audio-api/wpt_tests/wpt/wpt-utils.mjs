/**
 * WPT-only helpers. Not used in React Native production code — the native JSI
 * layer assumes callers pass valid Float32Array views. These guards exist so
 * WPT exception tests get TypeError (jsdom realm) instead of native crashes, and
 * SharedArrayBuffer-backed views are rejected per spec.
 */

export function assertFloat32Array(value, name, window) {
  const TypeErrorCtor = window?.TypeError ?? TypeError;

  // Cross-realm Float32Arrays (e.g. from a jsdom window) fail instanceof,
  // so also accept any object that looks like a Float32Array view.
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
