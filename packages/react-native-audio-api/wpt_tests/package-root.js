'use strict';

const fs = require('node:fs');
const path = require('node:path');
const { createRequire } = require('node:module');

/**
 * Root of the react-native-audio-api package whose JS build the WPT harness
 * loads. Defaults to the workspace package; set RN_AUDIO_API_APP_ROOT to an
 * app directory to test the version installed in its node_modules:
 *
 *   RN_AUDIO_API_APP_ROOT=/path/to/app yarn wpt:only
 *
 * @returns {string} absolute path to the package root
 */
function resolvePackageRoot() {
  const appRoot = process.env.RN_AUDIO_API_APP_ROOT;
  const workspaceRoot = path.resolve(__dirname, '..');
  if (!appRoot) {
    return workspaceRoot;
  }

  // Resolve with a require() anchored inside the app: package managers may
  // hoist the installed copy to an ancestor node_modules (e.g. the monorepo
  // root), so a hard-coded <app>/node_modules/<pkg> path is not reliable, and
  // resolving from this file would self-reference the workspace package.
  const appRequire = createRequire(
    path.join(path.resolve(appRoot), 'wpt-package-resolver.js')
  );
  const packageRoot = path.dirname(
    appRequire.resolve('react-native-audio-api/package.json')
  );
  if (packageRoot === workspaceRoot) {
    throw new Error(
      `RN_AUDIO_API_APP_ROOT=${appRoot} resolves react-native-audio-api to the ` +
        'workspace package itself. Pin a published version in the app and run yarn install.'
    );
  }
  if (!fs.existsSync(path.join(packageRoot, 'lib', 'commonjs'))) {
    throw new Error(
      `react-native-audio-api JS build not found at ${packageRoot}/lib/commonjs. ` +
        'Check RN_AUDIO_API_APP_ROOT and that the app has react-native-audio-api installed.'
    );
  }

  console.log(`[wpt] Using react-native-audio-api JS from ${packageRoot}`);
  return packageRoot;
}

module.exports = resolvePackageRoot();
