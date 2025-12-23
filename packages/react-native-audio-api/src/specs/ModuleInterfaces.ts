import type { TurboModule } from 'react-native';
import { AudioDevicesInfo, PermissionStatus } from '../system/types';

export type OptionsMap = {
  [key: string]: string | boolean | number | undefined;
};
export type NotificationOpResponse = { success: boolean; error?: string };
export type NotificationType = 'playback' | 'recording' | 'simple';

export interface Spec extends TurboModule {
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
    type: NotificationType,
    key: string
  ): Promise<NotificationOpResponse>;
  showNotification(
    key: string,
    options: OptionsMap
  ): Promise<NotificationOpResponse>;
  updateNotification(
    key: string,
    options: OptionsMap
  ): Promise<NotificationOpResponse>;
  hideNotification(key: string): Promise<NotificationOpResponse>;
  unregisterNotification(key: string): Promise<NotificationOpResponse>;
  isNotificationActive(key: string): Promise<boolean>;
}
