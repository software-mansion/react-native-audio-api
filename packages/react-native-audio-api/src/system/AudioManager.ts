import {
  SessionOptions,
  LockScreenInfo,
  RemoteEventName,
  RemoteEventCallback,
  RemoteCommandName,
  PermissionStatus,
} from './types';
import { NativeAudioAPIModule, eventEmitter } from '../specs';
import { EmitterSubscription } from 'react-native';

class AudioManager {
  setLockScreenInfo(info: LockScreenInfo) {
    NativeAudioAPIModule.setLockScreenInfo(info);
  }

  resetLockScreenInfo() {
    NativeAudioAPIModule.resetLockScreenInfo();
  }

  setAudioSessionOptions(options: SessionOptions) {
    NativeAudioAPIModule.setAudioSessionOptions(
      options.iosCategory ?? '',
      options.iosMode ?? '',
      options.iosOptions ?? []
    );
  }

  getDevicePreferredSampleRate(): number {
    return NativeAudioAPIModule.getDevicePreferredSampleRate();
  }

  observeAudioInterruptions(enabled: boolean) {
    NativeAudioAPIModule.observeAudioInterruptions(enabled);
  }

  observeVolumeChanges(enabled: boolean) {
    NativeAudioAPIModule.observeVolumeChanges(enabled);
  }

  enableRemoteCommand<Name extends RemoteCommandName>(name: Name): void {
    NativeAudioAPIModule.enableRemoteCommand(name, true);
  }

  enableRemoteEvent<Name extends RemoteEventName>(
    name: Name,
    callback?: RemoteEventCallback<Name>
  ): EmitterSubscription | null {
    let subscription = null;
    if (!callback) {
      return null;
    }
    switch (name) {
      case 'interruption':
        subscription = eventEmitter.addListener('onInterruption', callback);
        break;

      case 'routeChange':
        subscription = eventEmitter.addListener('onRouteChange', callback);
        break;

      case 'volumeChange':
        subscription = eventEmitter.addListener('onVolumeChange', callback);
        break;

      default:
        console.error('Unsupported RemoteControl action:', name);
    }
    return subscription;
  }

  async requestRecordingPermissions(): Promise<PermissionStatus> {
    return NativeAudioAPIModule.requestRecordingPermissions();
  }

  async checkRecordingPermissions(): Promise<PermissionStatus> {
    return NativeAudioAPIModule.checkRecordingPermissions();
  }
}

export default new AudioManager();
