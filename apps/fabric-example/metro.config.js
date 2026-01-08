const { getDefaultConfig, mergeConfig } = require('@react-native/metro-config');

const path = require('path');

const monorepoRoot = path.resolve(__dirname, '../..');
const appsRoot = path.resolve(monorepoRoot, 'apps');

/**
 * Metro configuration https://reactnative.dev/docs/metro
 *
 * @type {import('@react-native/metro-config').MetroConfig}
 */
const config = {
  projectRoot: __dirname,
  watchFolders: [monorepoRoot, appsRoot],
};

module.exports = mergeConfig(getDefaultConfig(__dirname), config);
