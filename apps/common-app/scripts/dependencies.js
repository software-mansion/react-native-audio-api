const fs = require('fs');
const path = require('path');

/**
 * Finds `node_modules/<name>` the way Node does, walking up from common-app, so
 * it holds for hoisted and isolated node_modules alike.
 *
 * @param {string} name
 */
function findLinkedRoot(name) {
  for (let dir = path.resolve(__dirname, '..'); ; dir = path.dirname(dir)) {
    const candidate = path.join(dir, 'node_modules', name);
    if (fs.existsSync(candidate)) {
      return candidate;
    }
    if (dir === path.dirname(dir)) {
      throw new Error(`common-app: cannot find dependency ${name}`);
    }
  }
}

/**
 * Follows the node_modules symlink of workspace packages, so Gradle and
 * CocoaPods register them under packages/, the same path editors use.
 * Other packages keep the node_modules path: under an isolated layout their
 * real path embeds a store hash that would churn Podfile.lock.
 *
 * @param {string} name
 */
function resolveDependencyRoot(name) {
  const linkedRoot = findLinkedRoot(name);
  const realRoot = fs.realpathSync(linkedRoot);
  const isWorkspacePackage = !realRoot.split(path.sep).includes('node_modules');
  return isWorkspacePackage ? realRoot : linkedRoot;
}

/**
 * @param {Object<string, string>} dependencies
 * @param {Set<string>} exclude
 */
function resolveDependencies(dependencies = {}, exclude) {
  return Object.fromEntries(
    Object.keys(dependencies)
      .filter((name) => !exclude.has(name))
      .map((name) => [name, { root: resolveDependencyRoot(name) }])
  );
}

/**
 * This function will return the dependencies from the common-app package that
 * aren't listed in the current app's package.json
 *
 * @param {string} currentAppDir - The current app directory (e.g. __dirname)
 * @param {string[]} exclude - The dependencies to exclude from the common-app
 */
function getDependencies(currentAppDir = '.', exclude = []) {
  const commonAppDir = path.resolve(__dirname, '..');
  const commonAppPkg = require(path.resolve(commonAppDir, 'package.json'));

  const currentAppPkg = require(path.resolve(currentAppDir, 'package.json'));

  const excludedDependencies = new Set([
    ...Object.keys(currentAppPkg.devDependencies),
    ...Object.keys(currentAppPkg.dependencies),
    ...exclude,
  ]);

  return {
    // Get all common-app dependencies that aren't already in the current app
    ...resolveDependencies(commonAppPkg.devDependencies, excludedDependencies),
    ...resolveDependencies(commonAppPkg.dependencies, excludedDependencies),
  };
}

module.exports = {
  getDependencies,
};
