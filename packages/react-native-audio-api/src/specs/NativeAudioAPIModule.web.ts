import type { Spec } from './ModuleInterfaces';
import { PermissionStatus } from '../system/types';

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
  registerNotification: mockAsync({ success: true }),
  showNotification: mockAsync({ success: true }),
  updateNotification: mockAsync({ success: true }),
  hideNotification: mockAsync({ success: true }),
  unregisterNotification: mockAsync({ success: true }),
  isNotificationActive: mockAsync(false),
};

export { NativeAudioAPIModule };
