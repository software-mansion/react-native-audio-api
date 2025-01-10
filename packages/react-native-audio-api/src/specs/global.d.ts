import type { IAudioContext } from '../interfaces';

type AudioAPIInstaller = {
  createAudioContext: () => IAudioContext;
};

declare global {
  function nativeCallSyncHook(): unknown;
  var __AudioAPIInstaller: AudioAPIInstaller;
}
