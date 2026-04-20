const { getDefaultConfig, mergeConfig } = require('@react-native/metro-config');
const path = require('path');

const monorepoRoot = path.resolve(__dirname, '../..');
const appsRoot = path.resolve(monorepoRoot, 'apps');

const defaultConfig = getDefaultConfig(__dirname);
/**
 * Metro configuration https://reactnative.dev/docs/metro
 *
 * @type {import('@react-native/metro-config').MetroConfig}
 */
const config = {
  projectRoot: __dirname,
  watchFolders: [monorepoRoot, appsRoot],
  resolver: {
    assetExts: [...defaultConfig.resolver.assetExts, 'ogg', 'flac', 'opus'],
  },
  /* we are rewriting requests because due to monorepo structure, the assets are found with '../../../' prefix
  and we redirect them to the correct path without relative prefixes */
  server: {
    rewriteRequestUrl: (url) => {
      if (!url.startsWith('/assets/../../')) {
        return url;
      }

      const queryIndex = url.indexOf('?');
      const pathname = queryIndex >= 0 ? url.substring(0, queryIndex) : url;
      const query = queryIndex >= 0 ? url.substring(queryIndex) : '';
      const separator = query ? '&' : '?';

      const relPath = pathname.startsWith('/assets/')
        ? pathname.substring('/assets/'.length)
        : `../../${pathname}`;

      const rewrittenUrl = `/assets${query}${separator}unstable_path=${encodeURIComponent(relPath)}`;

      return rewrittenUrl;
    },
  },
};

module.exports = mergeConfig(defaultConfig, config);
