const { getDefaultConfig, mergeConfig } = require('@react-native/metro-config');
const path = require('path');

const projectRoot = __dirname;
const repostioryRoot = path.resolve(__dirname, '../..');
// @ts-expect-error
const exclusionList = require('metro-config/src/defaults/exclusionList');
const escape = require('escape-string-regexp');

const duplicatedModules = ['react', 'react-dom', 'react-native'];

const defaultConfig = getDefaultConfig(__dirname);

/**
 * Metro configuration
 * https://facebook.github.io/metro/docs/configuration
 *
 * @type {import('metro-config').MetroConfig}
 */
const config = {
  projectRoot,
  watchFolders: [repostioryRoot],

  resolver: {
    assetExts: [...defaultConfig.resolver.assetExts, 'wav'],
    blacklistRE: exclusionList(
      duplicatedModules.map(
        (m) =>
          new RegExp(
            `^${escape(path.join(repostioryRoot, 'node_modules', m))}\\/.*$`
          )
      )
    ),

    extraNodeModules: duplicatedModules.reduce((acc, name) => {
      // @ts-expect-error
      acc[name] = path.join(__dirname, 'node_modules', name);
      return acc;
    }, {}),
  },
};

module.exports = mergeConfig(getDefaultConfig(__dirname), config);
