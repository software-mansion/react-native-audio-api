import { TurboModule } from 'react-native';
import { AudioDevicesInfo, PermissionStatus } from '../system/types';

// copy of spec from NativeAudioAPIModule.ts
type OptionsMap = {
  [key: string]: string | boolean | number | undefined;
};
type NotificationOpResponse = { success: boolean; error?: string };
type NotificationType = 'playback' | 'recording' | 'simple';

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
  setInputDevice(deviceId: string): Promise<boolean>;

  // New notification system
  showNotification(
    type: NotificationType,
    key: string,
    options: OptionsMap
  ): Promise<NotificationOpResponse>;
  hideNotification(key: string): Promise<NotificationOpResponse>;
  isNotificationActive(key: string): Promise<boolean>;
}

const mockAsync =
  <T>(value: T) =>
  () =>
    Promise.resolve(value);
const mockSync =
  <T>(value: T) =>
  () =>
    value;

const NativeAudioAPIModule: Spec = {
  install: mockSync(true),
  getDevicePreferredSampleRate: mockSync(0),
  setAudioSessionActivity: mockAsync(true),
  setAudioSessionOptions: mockSync({}),
  disableSessionManagement: mockSync({}),
  observeAudioInterruptions: mockSync({}),
  activelyReclaimSession: mockSync({}),
  observeVolumeChanges: mockSync({}),
  requestRecordingPermissions: mockAsync('Granted' as PermissionStatus),
  checkRecordingPermissions: mockAsync('Granted' as PermissionStatus),
  requestNotificationPermissions: mockAsync('Granted' as PermissionStatus),
  checkNotificationPermissions: mockAsync('Granted' as PermissionStatus),
  getDevicesInfo: mockAsync({
    availableInputs: [],
    availableOutputs: [],
    currentInputs: [],
    currentOutputs: [],
  }),
  setInputDevice: mockAsync(true),
  showNotification: mockAsync({ success: true }),
  hideNotification: mockAsync({ success: true }),
  isNotificationActive: mockAsync(false),
};

export { NativeAudioAPIModule };
