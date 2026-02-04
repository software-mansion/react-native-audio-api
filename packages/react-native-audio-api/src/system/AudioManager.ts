import { AudioEventEmitter, AudioEventSubscription } from '../events';
import { SystemEventCallback, SystemEventName } from '../events/types';
import { NativeAudioAPIModule } from '../specs';
import { parseNativeError } from './errors';
import {
  AudioDevicesInfo,
  IAudioManager,
  PermissionStatus,
  SessionOptions,
} from './types';

class AudioManager implements IAudioManager {
  private readonly audioEventEmitter: AudioEventEmitter;
  constructor() {
    this.audioEventEmitter = new AudioEventEmitter(global.AudioEventEmitter);
  }

  getDevicePreferredSampleRate(): number {
    return NativeAudioAPIModule.getDevicePreferredSampleRate();
  }

  async setAudioSessionActivity(enabled: boolean): Promise<boolean> {
    try {
      const success =
        await NativeAudioAPIModule.setAudioSessionActivity(enabled);

      return success;
    } catch (error) {
      throw parseNativeError(error);
    }
  }

  setAudioSessionOptions(options: SessionOptions) {
    NativeAudioAPIModule.setAudioSessionOptions(
      options.iosCategory ?? '',
      options.iosMode ?? '',
      options.iosOptions ?? [],
      options.iosAllowHaptics ?? false
    );
  }

  disableSessionManagement() {
    NativeAudioAPIModule.disableSessionManagement();
  }

  observeAudioInterruptions(enabled: boolean) {
    NativeAudioAPIModule.observeAudioInterruptions(enabled);
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

  async setInputDevice(deviceId: string): Promise<boolean> {
    return NativeAudioAPIModule.setInputDevice(deviceId);
  }
}

export default new AudioManager();
