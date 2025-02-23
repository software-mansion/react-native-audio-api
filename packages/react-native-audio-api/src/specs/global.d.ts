/* eslint-disable no-var */
import type { IAudioContext, IAudioControls } from '../interfaces';

type AudioAPIInstaller = {
  createAudioContext: (sampleRate?: number) => IAudioContext;
  getAudioControls: () => IAudioControls;
};

declare global {
  function nativeCallSyncHook(): unknown;
  var __AudioAPIInstaller: AudioAPIInstaller;
}
/* eslint-disable no-var */
