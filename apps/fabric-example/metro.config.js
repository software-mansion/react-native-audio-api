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
  server: {
    rewriteRequestUrl: (url) => {
      if (!url.startsWith('/assets/../../')) {
        return url;
      }

      // return url.replace('/assets/../../', '/');
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

module.exports = mergeConfig(getDefaultConfig(__dirname), config);
