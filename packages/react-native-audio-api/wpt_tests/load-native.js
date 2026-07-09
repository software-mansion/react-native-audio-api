'use strict';

const fs = require('node:fs');
const path = require('node:path');

function getCandidates() {
  const packageRoot = path.resolve(__dirname, '..');
  const bindingsName = 'rnaudioapi_node_bindings.node';

  return [
    // Current layout after node -> wpt_tests rename.
    path.join(__dirname, 'build', 'Release', bindingsName),
    path.join(__dirname, 'build', 'Debug', bindingsName),
    // Backward compatibility with older package layouts.
    path.join(packageRoot, 'node', 'build', 'Release', bindingsName),
    path.join(packageRoot, 'node', 'build', 'Debug', bindingsName),
    path.join(packageRoot, 'build', 'Release', bindingsName),
    path.join(packageRoot, 'build', 'Debug', bindingsName),
  ];
}

function loadNative() {
  for (const filePath of getCandidates()) {
    if (fs.existsSync(filePath)) {
      // eslint-disable-next-line @typescript-eslint/no-var-requires, import/no-dynamic-require, global-require
      return require(filePath);
    }
  }
  return null;
}

module.exports = loadNative;
