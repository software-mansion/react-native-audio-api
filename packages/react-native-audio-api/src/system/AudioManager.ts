import { AudioOptions, LockScreenInfo, PermissionStatus } from './types';
import { SystemEventName, SystemEventCallback } from '../events/types';
import { NativeAudioAPIModule } from '../specs';
import { AudioEventEmitter, AudioEventSubscription } from '../events';

if (global.AudioEventEmitter == null) {
  if (!NativeAudioAPIModule) {
    throw new Error(
      `Failed to install react-native-audio-api: The native module could not be found.`
    );
  }

  NativeAudioAPIModule.install();
}

class AudioManager {
  private readonly audioEventEmitter: AudioEventEmitter;
  constructor() {
    this.audioEventEmitter = new AudioEventEmitter(global.AudioEventEmitter);
  }

  setLockScreenInfo(info: LockScreenInfo) {
    NativeAudioAPIModule!.setLockScreenInfo(info);
  }

  resetLockScreenInfo() {
    NativeAudioAPIModule!.resetLockScreenInfo();
  }

  getDevicePreferredSampleRate(): number {
    return NativeAudioAPIModule!.getDevicePreferredSampleRate();
  }

  observeAudioInterruptions(enabled: boolean) {
    NativeAudioAPIModule!.observeAudioInterruptions(enabled);
  }

  setAudioOptions(observeAudioInterruption = true, options?: AudioOptions) {
    NativeAudioAPIModule!.setAudioOptions(observeAudioInterruption, options);
  }

  abandonAudioFocus() {
    NativeAudioAPIModule!.abandonAudioFocus();
  }

  observeVolumeChanges(enabled: boolean) {
    NativeAudioAPIModule!.observeVolumeChanges(enabled);
  }

  enableSystemEvent<Name extends SystemEventName>(
    name: Name,
    callback?: SystemEventCallback<Name>,
    enabled = true
  ): AudioEventSubscription | null {
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
