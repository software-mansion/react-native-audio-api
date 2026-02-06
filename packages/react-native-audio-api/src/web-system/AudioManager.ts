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
  setAudioSessionActivity = mockAsync(true);
  setAudioSessionOptions = mockSync({});
  disableSessionManagement = mockSync({});
  observeAudioInterruptions = mockSync(true);
  activelyReclaimSession = mockSync({});
  observeVolumeChanges = mockSync({});
  addSystemEventListener = mockSync(undefined);
  requestRecordingPermissions = mockAsync('Granted' as PermissionStatus);
  checkRecordingPermissions = mockAsync('Granted' as PermissionStatus);
  requestNotificationPermissions = mockAsync('Granted' as PermissionStatus);
  checkNotificationPermissions = mockAsync('Granted' as PermissionStatus);
  setInputDevice = mockAsync(true);
  getDevicesInfo = mockAsync({
    availableInputs: [],
    availableOutputs: [],
    currentInputs: [],
    currentOutputs: [],
  });
}

export default new AudioManager();
