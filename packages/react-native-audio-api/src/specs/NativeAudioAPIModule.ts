'use strict';
import { TurboModuleRegistry } from 'react-native';
import type { TurboModule } from 'react-native';
import { PermissionStatus, AudioDevicesInfo } from '../system/types';

interface Spec extends TurboModule {
  install(): boolean;
  getDevicePreferredSampleRate(): number;

  // AVAudioSession management
  setAudioSessionActivity(enabled: boolean): Promise<boolean>;
  setAudioSessionOptions(
    category: string,
    mode: string,
    options: Array<string>,
    allowHaptics: boolean
  ): void;
  disableSessionManagement(): void;

  // Remote commands, system events and interruptions
  observeAudioInterruptions(enabled: boolean): void;
  activelyReclaimSession(enabled: boolean): void;
  observeVolumeChanges(enabled: boolean): void;

  // Permissions
  requestRecordingPermissions(): Promise<PermissionStatus>;
  checkRecordingPermissions(): Promise<PermissionStatus>;
  requestNotificationPermissions(): Promise<PermissionStatus>;
  checkNotificationPermissions(): Promise<PermissionStatus>;

  // Audio devices
  getDevicesInfo(): Promise<AudioDevicesInfo>;

  // New notification system
  registerNotification(
    type: string,
    key: string
  ): Promise<{ success: boolean; error?: string }>;
  showNotification(
    key: string,
    options: { [key: string]: string | boolean | number | undefined }
  ): Promise<{ success: boolean; error?: string }>;
  updateNotification(
    key: string,
    options: { [key: string]: string | boolean | number | undefined }
  ): Promise<{ success: boolean; error?: string }>;
  hideNotification(key: string): Promise<{ success: boolean; error?: string }>;
  unregisterNotification(
    key: string
  ): Promise<{ success: boolean; error?: string }>;
  isNotificationActive(key: string): Promise<boolean>;
}

const NativeAudioAPIModule = TurboModuleRegistry.get<Spec>('AudioAPIModule');

export { NativeAudioAPIModule };
