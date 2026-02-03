'use strict';
import type { TurboModule } from 'react-native';
import { TurboModuleRegistry } from 'react-native';
import { AudioDevicesInfo, PermissionStatus } from '../system/types';

type OptionsMap = { [key: string]: string | boolean | number | undefined };
type NotificationOpResponse = { success: boolean; error?: string };
type NotificationType = 'playback' | 'recording' | 'simple';
type AudioFocusType =
  | 'gain'
  | 'gainTransient'
  | 'gainTransientExclusive'
  | 'gainTransientMayDuck';

interface Spec extends TurboModule {
  install(): boolean;
  getDevicePreferredSampleRate(): number;

  // AVAudioSession management
  setAudioSessionActivity(enabled: boolean): Promise<boolean>;
  setAudioSessionOptions(
    category: string,
    mode: string,
    options: Array<string>,
    allowHaptics: boolean,
    notifyOthersOnDeactivation: boolean
  ): void;
  disableSessionManagement(): void;

  // system events and interruptions
  observeAudioInterruptions(focusType: AudioFocusType, enabled: boolean): void;
  activelyReclaimSession(enabled: boolean): void;
  observeVolumeChanges(enabled: boolean): void;

  // Permissions
  requestRecordingPermissions(): Promise<PermissionStatus>;
  checkRecordingPermissions(): Promise<PermissionStatus>;
  requestNotificationPermissions(): Promise<PermissionStatus>;
  checkNotificationPermissions(): Promise<PermissionStatus>;

  // Audio devices
  getDevicesInfo(): Promise<AudioDevicesInfo>;

  // Notification system
  showNotification(
    type: NotificationType,
    key: string,
    options: OptionsMap
  ): Promise<NotificationOpResponse>;
  hideNotification(key: string): Promise<NotificationOpResponse>;
  isNotificationActive(key: string): Promise<boolean>;
}

const NativeAudioAPIModule = TurboModuleRegistry.get<Spec>('AudioAPIModule');

export { NativeAudioAPIModule };
