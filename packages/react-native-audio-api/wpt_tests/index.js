'use strict';

const Module = require('node:module');
const path = require('node:path');

require('./native-module');

const mediaDevices = {
  async getUserMedia() {
    throw new Error('mediaDevices.getUserMedia is not supported in Node phase-1 bindings.');
  },
};

function loadTypeScriptApi() {
  // eslint-disable-next-line @typescript-eslint/no-var-requires, import/no-dynamic-require, global-require
  return require(path.join(__dirname, 'wpt-api.js'));
}

function installReactNativeNodeShim() {
  const globalAny = global;
  if (globalAny.__RNAudioApiReactNativeShimInstalled === true) {
    return;
  }

  const nativeModuleStub = {
    install() {
      return true;
    },
    getDevicePreferredSampleRate() {
      return 44100;
    },
    isFfmpegEnabled() {
      return false;
    },
  };

  const reactNativeShim = {
    TurboModuleRegistry: {
      get() {
        return nativeModuleStub;
      },
      getEnforcing() {
        return nativeModuleStub;
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
module.exports = {
  ...typeScriptApi,
  mediaDevices,
};
