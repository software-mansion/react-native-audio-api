/* eslint-disable no-var */
import type { IAudioContext } from '../interfaces';

type AudioAPIInstaller = {
  createAudioContext: (sampleRate?: number) => IAudioContext;
};

declare global {
  function nativeCallSyncHook(): unknown;
  var __AudioAPIInstaller: AudioAPIInstaller;
}
/* eslint-disable no-var */
