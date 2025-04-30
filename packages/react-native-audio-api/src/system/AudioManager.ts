import {
  SessionOptions,
  LockScreenInfo,
  RemoteEventName,
  RemoteEventCallback,
  RemoteCommandName,
  PermissionStatus,
} from './types';
import { AudioManagerModule, eventEmitter } from '../specs';
import { EmitterSubscription } from 'react-native';

class AudioManager {
  setLockScreenInfo(info: LockScreenInfo) {
    AudioManagerModule.setLockScreenInfo(info);
  }

  resetLockScreenInfo() {
    AudioManagerModule.resetLockScreenInfo();
  }

  setAudioSessionOptions(options: SessionOptions) {
    AudioManagerModule.setAudioSessionOptions(
      options.iosCategory ?? '',
      options.iosMode ?? '',
      options.iosOptions ?? []
    );
  }

  getDevicePreferredSampleRate(): number {
    return AudioManagerModule.getDevicePreferredSampleRate();
  }

  observeAudioInterruptions(enabled: boolean) {
    AudioManagerModule.observeAudioInterruptions(enabled);
  }

  observeVolumeChanges(enabled: boolean) {
    AudioManagerModule.observeVolumeChanges(enabled);
  }

  enableRemoteCommand<Name extends RemoteCommandName>(name: Name): void {
    AudioManagerModule.enableRemoteCommand(name, true);
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
    return AudioManagerModule.requestRecordingPermissions();
  }

  async checkRecordingPermissions(): Promise<PermissionStatus> {
    return AudioManagerModule.checkRecordingPermissions();
  }
}

export default new AudioManager();
