import type { IAudioManager, PermissionStatus } from '../system/types';

const mockAsync =
  <T>(value: T) =>
  () =>
    Promise.resolve(value);
const mockSync =
  <T>(value: T) =>
  () =>
    value;

class AudioManager implements IAudioManager {
  getDevicePreferredSampleRate = mockSync(44100);
  getSystemVolume = mockSync(1);
  setAudioSessionActivity = mockAsync(undefined);
  setSystemOptions = mockSync({});
  disableSessionManagement = mockSync({});
  observeAudioInterruptions = mockSync(true);
  activelyReclaimSession = mockSync({});
  observeVolumeChanges = mockSync({});
  addSystemEventListener = mockSync(undefined);
  requestRecordingPermissions = mockAsync<PermissionStatus>('Granted');
  checkRecordingPermissions = mockAsync<PermissionStatus>('Granted');
  requestNotificationPermissions = mockAsync<PermissionStatus>('Granted');
  checkNotificationPermissions = mockAsync<PermissionStatus>('Granted');
  setInputDevice = mockAsync(undefined);
  setCommunicationDevice = mockAsync(undefined);
  getCommunicationDevice = mockAsync(null);
  getDevicesInfo = mockAsync({
    availableInputs: [],
    availableOutputs: [],
    currentInputs: [],
    currentOutputs: [],
  });
}

export default new AudioManager();
