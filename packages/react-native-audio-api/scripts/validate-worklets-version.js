'use strict';

const semverGte = require('semver/functions/gte');

const MIN_WORKLETS_VERSION = '0.6.0';


/**
 * Checks if the installed version of react-native-worklets is compatible with react-native-audio-api.
 * @returns {boolean} True if the version is compatible, false otherwise.
 */
function validateVersion() {
  let workletsVersion;
  try {
    const { version } = require('react-native-worklets/package.json');
    workletsVersion = version;
  } catch (e) {
    return false;
  }

  return semverGte(workletsVersion, MIN_WORKLETS_VERSION);
}

if (!validateVersion()) {
  console.warn(
    '[RNAudioApi] Incompatible version of react-native-audio-worklets detected. Please install a compatible version if you want to use worklet nodes in react-native-audio-api.'
  );
  process.exit(1);
}
