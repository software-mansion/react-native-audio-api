'use strict';

const fs = require('node:fs');
const Module = require('node:module');
const path = require('node:path');

require('./native-module');

const mediaDevices = {
  async getUserMedia() {
    throw new Error('mediaDevices.getUserMedia is not supported in Node phase-1 bindings.');
  },
};

function loadTypeScriptApi() {
  const packageRoot = path.resolve(__dirname, '..');
  const candidates = [
    path.join(packageRoot, 'lib', 'commonjs', 'api.js'),
    path.join(packageRoot, 'lib', 'commonjs', 'api', 'index.js'),
  ];

  for (const filePath of candidates) {
    if (!fs.existsSync(filePath)) {
      continue;
    }

    try {
      // eslint-disable-next-line @typescript-eslint/no-var-requires, import/no-dynamic-require, global-require
      return require(filePath);
    } catch {
      // Try next candidate path.
    }
  }

  return null;
}

function installReactNativeNodeShim() {
  const globalAny = global;
  if (globalAny.__RNAudioApiReactNativeShimInstalled === true) {
    return;
  }

  const reactNativeShim = {
    TurboModuleRegistry: {
      get() {
        return null;
      },
    },
    Platform: { OS: 'ios' },
    Image: {
      resolveAssetSource(assetId) {
        return { uri: String(assetId) };
      },
    },
  };

  const originalLoad = Module._load;
  Module._load = function patchedLoad(request, parent, isMain) {
    if (request === 'react-native') {
      return reactNativeShim;
    }
    return originalLoad.call(this, request, parent, isMain);
  };

  if (globalAny.__DEV__ == null) {
    globalAny.__DEV__ = false;
  }

  if (globalAny.__RNAudioAPINativeModule == null) {
    globalAny.__RNAudioAPINativeModule = {
      getDevicePreferredSampleRate() {
        return 44100;
      },
    };
  }

  globalAny.__RNAudioApiReactNativeShimInstalled = true;
}

installReactNativeNodeShim();

const typeScriptApi = loadTypeScriptApi();
if (typeScriptApi != null) {
  module.exports = {
    ...typeScriptApi,
    mediaDevices: typeScriptApi.mediaDevices || mediaDevices,
  };
} else {
  // eslint-disable-next-line global-require
  module.exports = require('./index-fallback');
}
