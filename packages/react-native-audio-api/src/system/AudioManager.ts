import { AudioEventEmitter, AudioEventSubscription } from '../events';
import { SystemEventCallback, SystemEventName } from '../events/types';
import { NativeAudioAPIModule } from '../specs';
import { parseNativeError } from './errors';
import {
  AudioDevicesInfo,
  AudioFocusType,
  IAudioManager,
  PermissionStatus,
  SessionOptions,
} from './types';

class AudioManager implements IAudioManager {
  private readonly audioEventEmitter: AudioEventEmitter;
  constructor() {
    this.audioEventEmitter = new AudioEventEmitter(
      globalThis.AudioEventEmitter
    );
  }

  getDevicePreferredSampleRate(): number {
    return NativeAudioAPIModule.getDevicePreferredSampleRate();
  }

  /**
   * Activates or deactivates the audio session.
   *
   * Resolves when the session activity was set successfully. On failure it
   * rejects with a {@link SessionActivationError} carrying the native error
   * details (`nativeErrorInfo`) when available.
   */
  async setAudioSessionActivity(enabled: boolean): Promise<void> {
    try {
      await NativeAudioAPIModule.setAudioSessionActivity(enabled);
    } catch (error) {
      throw parseNativeError(error);
    }
  }

  setAudioSessionOptions(options: SessionOptions) {
    NativeAudioAPIModule.setAudioSessionOptions(
      options.iosCategory ?? '',
      options.iosMode ?? '',
      options.iosOptions ?? [],
      options.iosAllowHaptics ?? false,
      options.iosNotifyOthersOnDeactivation ?? true
    );
  }

  disableSessionManagement() {
    NativeAudioAPIModule.disableSessionManagement();
  }

  observeAudioInterruptions(param: AudioFocusType | boolean | null) {
    if (typeof param === 'string') {
      NativeAudioAPIModule.observeAudioInterruptions(param, true);
    } else {
      // audiofocusgain as default value if not provided
      NativeAudioAPIModule.observeAudioInterruptions('gain', param === true);
    }
  }

  /**
   * @param enabled - Whether to actively reclaim the session or not
   * @experimental more aggressively try to reactivate the audio session during interruptions.
   * It is subject to change in the future and might be removed.
   *
   * In some cases (depends on app session settings and other apps using audio) system may never
   * send the `interruption ended` event. This method will check if any other audio is playing
   * and try to reactivate the audio session, as soon as there is "silence".
   * Although this might change the expected behavior.
   *
   * Internally method uses `AVAudioSessionSilenceSecondaryAudioHintNotification` as well as
   * interval polling to check if other audio is playing.
   */
  activelyReclaimSession(enabled: boolean) {
    NativeAudioAPIModule.activelyReclaimSession(enabled);
  }

  observeVolumeChanges(enabled: boolean) {
    NativeAudioAPIModule.observeVolumeChanges(enabled);
  }

  /**
   * Synchronously reads the current system output volume as a 0..1 value, on
   * the same scale the `volumeChange` event reports
   *
   * On iOS, a cold read before any volume-change event can report a stale or
   * zero value on some OS versions; treat the event stream as the truth once it
   * speaks.
   */
  getSystemVolume(): number {
    return NativeAudioAPIModule.getSystemVolume();
  }

  addSystemEventListener<Name extends SystemEventName>(
    name: Name,
    callback: SystemEventCallback<Name>
  ): AudioEventSubscription {
    return this.audioEventEmitter.addAudioEventListener(name, callback);
  }

  async requestRecordingPermissions(): Promise<PermissionStatus> {
    return NativeAudioAPIModule.requestRecordingPermissions();
  }

  async checkRecordingPermissions(): Promise<PermissionStatus> {
    return NativeAudioAPIModule.checkRecordingPermissions();
  }

  async requestNotificationPermissions(): Promise<PermissionStatus> {
    return NativeAudioAPIModule.requestNotificationPermissions();
  }

  async checkNotificationPermissions(): Promise<PermissionStatus> {
    return NativeAudioAPIModule.checkNotificationPermissions();
  }

  async getDevicesInfo(): Promise<AudioDevicesInfo> {
    return NativeAudioAPIModule.getDevicesInfo();
  }

  /**
   * Selects the given device as the current audio input.
   *
   * Resolves when the input device was set successfully and rejects when the
   * device cannot be found or the system fails to switch to it.
   *
   * On iOS the running session is rerouted right away. On Android the device is
   * bound while a capture stream opens, so the selection applies to recorders
   * started afterwards, and calling this while a recorder is running or paused
   * rejects rather than deferring the switch silently. Android also needs the
   * AAudio backend: a recorder that can only open through OpenSL ES fails to
   * start with an explanatory message instead of recording from the wrong
   * device.
   */
  async setInputDevice(deviceId: string): Promise<void> {
    await NativeAudioAPIModule.setInputDevice(deviceId);
  }
}

export default new AudioManager();
