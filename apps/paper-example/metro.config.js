const { getDefaultConfig, mergeConfig } = require('@react-native/metro-config');
const {
  wrapWithAudioAPIMetroConfig,
} = require('react-native-audio-api/metro-config');

const path = require('path');

const root = path.resolve(__dirname, '../..');

/**
 * Metro configuration https://reactnative.dev/docs/metro
 *
 * @type {import('@react-native/metro-config').MetroConfig}
 */
const config = {
  watchFolders: [root],
};

module.exports = wrapWithAudioAPIMetroConfig(
  mergeConfig(getDefaultConfig(__dirname), config)
);
