import {
  SessionOptions,
  LockScreenInfo,
  PermissionStatus,
  AudioDevicesInfo,
} from './types';
import {
  SystemEventName,
  SystemEventCallback,
  RemoteCommandEventName,
} from '../events/types';
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

  getDevicePreferredSampleRate(): number {
    return NativeAudioAPIModule!.getDevicePreferredSampleRate();
  }

  setAudioSessionActivity(enabled: boolean): Promise<boolean> {
    return NativeAudioAPIModule!.setAudioSessionActivity(enabled);
  }

  setAudioSessionOptions(options: SessionOptions) {
    NativeAudioAPIModule!.setAudioSessionOptions(
      options.iosCategory ?? '',
      options.iosMode ?? '',
      options.iosOptions ?? [],
      options.iosAllowHaptics ?? false
    );
  }

  setLockScreenInfo(info: LockScreenInfo) {
    NativeAudioAPIModule!.setLockScreenInfo(info);
  }

  resetLockScreenInfo() {
    NativeAudioAPIModule!.resetLockScreenInfo();
  }

  observeAudioInterruptions(enabled: boolean) {
    NativeAudioAPIModule!.observeAudioInterruptions(enabled);
  }

  observeVolumeChanges(enabled: boolean) {
    NativeAudioAPIModule!.observeVolumeChanges(enabled);
  }

  enableRemoteCommand(name: RemoteCommandEventName, enabled: boolean) {
    NativeAudioAPIModule!.enableRemoteCommand(name, enabled);
  }

  addSystemEventListener<Name extends SystemEventName>(
    name: Name,
    callback: SystemEventCallback<Name>
  ): AudioEventSubscription {
    return this.audioEventEmitter.addAudioEventListener(name, callback);
  }

  async requestRecordingPermissions(): Promise<PermissionStatus> {
    return NativeAudioAPIModule!.requestRecordingPermissions();
  }

  async checkRecordingPermissions(): Promise<PermissionStatus> {
    return NativeAudioAPIModule!.checkRecordingPermissions();
  }

  async getDevicesInfo(): Promise<AudioDevicesInfo> {
    return NativeAudioAPIModule!.getDevicesInfo();
  }
}

export default new AudioManager();
