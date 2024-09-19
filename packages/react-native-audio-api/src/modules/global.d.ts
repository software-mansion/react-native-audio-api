import type { BaseAudioContext } from '../types';

type AudioAPIInstaller = {
  createAudioContext: () => BaseAudioContext;
};

declare global {
  function nativeCallSyncHook(): unknown;
  var __AudioAPIInstaller: AudioAPI;
}
