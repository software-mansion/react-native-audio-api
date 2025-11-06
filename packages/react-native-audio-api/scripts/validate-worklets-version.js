'use strict';
const semverPrerelease = require('semver/functions/prerelease');
const validWorkletsVersions = [
  '0.6.0',
  '0.6.1',
];

function validateVersion() {
  let workletsVersion;
  try {
    const { version } = require('react-native-worklets/package.json');
    workletsVersion = version;
  } catch (e) {
    return false;
  }
  if (semverPrerelease(workletsVersion)) {
    return true;
  }

  return validWorkletsVersions.includes(workletsVersion);
}

if (!validateVersion()) {
  console.warn(
    '[RNAudioApi] Incompatible version of react-native-audio-worklets detected. Please install a compatible version if you want to use worklet nodes in react-native-audio-api.',
  );
  process.exit(1);
}
