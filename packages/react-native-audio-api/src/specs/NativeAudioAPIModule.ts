'use strict';
import {
  TurboModuleRegistry,
  NativeEventEmitter,
  Platform,
} from 'react-native';
import type { TurboModule, NativeModule } from 'react-native';
import { PermissionStatus } from '../system/types';

interface Spec extends TurboModule {
  install(): boolean;

  setLockScreenInfo(info: {
    [key: string]: string | boolean | number | undefined;
  }): void;
  resetLockScreenInfo(): void;
  enableRemoteCommand(name: string, enabled: boolean): void;
  setAudioSessionOptions(
    category: string,
    mode: string,
    options: Array<string>
  ): void;
  getDevicePreferredSampleRate(): number;
  observeAudioInterruptions(enabled: boolean): void;
  observeVolumeChanges(enabled: boolean): void;
  requestRecordingPermissions(): Promise<PermissionStatus>;
  checkRecordingPermissions(): Promise<PermissionStatus>;
}

const NativeAudioAPIModule =
  TurboModuleRegistry.getEnforcing<Spec>('AudioAPIModule');

const eventEmitter = new NativeEventEmitter(
  Platform.OS === 'android'
    ? undefined
    : (NativeAudioAPIModule as unknown as NativeModule)
);

export { NativeAudioAPIModule, eventEmitter };
