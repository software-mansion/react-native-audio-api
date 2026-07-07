'use strict';

const loadNative = require('./load-native');

const nativeBinding = loadNative();
if (nativeBinding == null || nativeBinding.nativeModule == null) {
  throw new Error('Failed to load Node native bindings for react-native-audio-api.');
}

const install = nativeBinding.nativeModule.install;
if (typeof install !== 'function') {
  throw new Error('Node native bindings are missing install() entrypoint.');
}

const installed = Boolean(install());
if (!installed) {
  throw new Error('Node native bindings install() returned false.');
}

module.exports = { installed };
