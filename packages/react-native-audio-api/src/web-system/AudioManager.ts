import type {
  CommunicationDevice,
  IAudioManager,
  PermissionStatus,
} from '../system/types';

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
  setInputDevice = mockAsync(undefined);
  setCommunicationDevice = (_device: CommunicationDevice) =>
    Promise.reject(new Error('setCommunicationDevice is not supported on web'));

  getCommunicationDevice = () =>
    Promise.reject(new Error('getCommunicationDevice is not supported on web'));

  getDevicesInfo = mockAsync({
    availableInputs: [],
    availableOutputs: [],
    currentInputs: [],
    currentOutputs: [],
  });
}

export default new AudioManager();
