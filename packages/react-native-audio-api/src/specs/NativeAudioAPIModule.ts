'use strict';
import { TurboModuleRegistry } from 'react-native';
import type { TurboModule } from 'react-native';
import { PermissionStatus, AudioAttributeType } from '../system/types';

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
  requestAudioFocus(options: {
    [key: string]: string | boolean | number | AudioAttributeType | undefined;
  }): void;
  abandonAudioFocus(): void;
  observeVolumeChanges(enabled: boolean): void;
  requestRecordingPermissions(): Promise<PermissionStatus>;
  checkRecordingPermissions(): Promise<PermissionStatus>;
}

const NativeAudioAPIModule = TurboModuleRegistry.get<Spec>('AudioAPIModule');

export { NativeAudioAPIModule };
