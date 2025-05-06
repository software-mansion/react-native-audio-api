import { SessionOptions, LockScreenInfo, PermissionStatus } from './types';
import { SystemEventName, SystemEventCallback } from '../events/types';
import { NativeAudioAPIModule } from '../specs';
import { IAudioEventEmitter } from '../interfaces';

if (global.AudioEventEmitter == null) {
  if (!NativeAudioAPIModule) {
    throw new Error(
      `Failed to install react-native-audio-api: The native module could not be found.`
    );
  }

  NativeAudioAPIModule.install();
}

class AudioManager {
  private readonly audioEventEmitter: IAudioEventEmitter;
  constructor() {
    this.audioEventEmitter = global.AudioEventEmitter;
  }

  setLockScreenInfo(info: LockScreenInfo) {
    NativeAudioAPIModule!.setLockScreenInfo(info);
  }

  resetLockScreenInfo() {
    NativeAudioAPIModule!.resetLockScreenInfo();
  }

  setAudioSessionOptions(options: SessionOptions) {
    NativeAudioAPIModule!.setAudioSessionOptions(
      options.iosCategory ?? '',
      options.iosMode ?? '',
      options.iosOptions ?? []
    );
  }

  getDevicePreferredSampleRate(): number {
    return NativeAudioAPIModule!.getDevicePreferredSampleRate();
  }

  observeAudioInterruptions(enabled: boolean) {
    NativeAudioAPIModule!.observeAudioInterruptions(enabled);
  }

  observeVolumeChanges(enabled: boolean) {
    NativeAudioAPIModule!.observeVolumeChanges(enabled);
  }

  enableSystemEvent<Name extends SystemEventName>(
    name: Name,
    callback?: SystemEventCallback<Name>,
    enabled = true
  ): number | null {
    NativeAudioAPIModule!.enableRemoteCommand(name, enabled);

    if (!enabled || !callback) {
      return null;
    }

    return this.audioEventEmitter.addAudioEventListener(name, callback);
  }

  async requestRecordingPermissions(): Promise<PermissionStatus> {
    return NativeAudioAPIModule!.requestRecordingPermissions();
  }

  async checkRecordingPermissions(): Promise<PermissionStatus> {
    return NativeAudioAPIModule!.checkRecordingPermissions();
  }
}

export default new AudioManager();
