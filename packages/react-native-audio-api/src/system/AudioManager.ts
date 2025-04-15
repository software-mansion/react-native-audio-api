import {
  SessionOptions,
  LockScreenInfo,
  RemoteControlEventName,
  InterruptionEventName,
  RemoteControlEmptyEventType,
  RemoteControlEventType,
  OnInterruptionEventType,
  OnRouteChangeEventType,
} from './types';
import { AudioManagerModule, eventEmitter } from '../specs';
import { EmitterSubscription } from 'react-native';

type AudioManagerEventType =
  | RemoteControlEmptyEventType
  | RemoteControlEventType
  | OnInterruptionEventType
  | OnRouteChangeEventType;

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
      options.iosOptions ?? [],
      options.active ?? true
    );
  }

  getDevicePreferredSampleRate(): number {
    return AudioManagerModule.getDevicePreferredSampleRate();
  }

  observeAudioInterruptions(enabled: boolean) {
    AudioManagerModule.observeAudioInterruptions(enabled);
  }

  enableRemoteCommand(
    name: RemoteControlEventName | InterruptionEventName,
    enabled: boolean,
    callback?: (event: AudioManagerEventType) => void
  ): EmitterSubscription | null {
    AudioManagerModule.enableRemoteCommand(name, enabled);

    let subscription = null;

    if (enabled && callback) {
      switch (name) {
        case 'play':
          subscription = eventEmitter.addListener('onRemotePlay', callback);
          break;

        case 'pause':
          subscription = eventEmitter.addListener('onRemotePause', callback);
          break;

        case 'stop':
          subscription = eventEmitter.addListener('onRemoteStop', callback);
          break;

        case 'togglePlayPause':
          subscription = eventEmitter.addListener(
            'onRemoteTogglePlayPause',
            callback
          );
          break;

        case 'changePlaybackRate':
          subscription = eventEmitter.addListener(
            'onRemoteChangePlaybackRate',
            callback
          );
          break;

        case 'nextTrack':
          subscription = eventEmitter.addListener(
            'onRemoteNextTrack',
            callback
          );
          break;

        case 'previousTrack':
          subscription = eventEmitter.addListener(
            'onRemotePreviousTrack',
            callback
          );
          break;

        case 'skipForward':
          subscription = eventEmitter.addListener(
            'onRemoteSkipForward',
            callback
          );
          break;

        case 'skipBackward':
          subscription = eventEmitter.addListener(
            'onRemoteSkipBackward',
            callback
          );
          break;

        case 'seekForward':
          subscription = eventEmitter.addListener(
            'onRemoteSeekForward',
            callback
          );
          break;

        case 'seekBackward':
          subscription = eventEmitter.addListener(
            'onRemoteSeekBackward',
            callback
          );
          break;

        case 'changePlaybackPosition':
          subscription = eventEmitter.addListener(
            'onRemoteChangePlaybackPosition',
            callback
          );
          break;

        case 'interruption':
          subscription = eventEmitter.addListener('onInterruption', callback);
          break;

        case 'routeChange':
          subscription = eventEmitter.addListener('onRouteChange', callback);
          break;

        default:
          console.error('Unsupported RemoteControl action:', name);
      }
    }

    return subscription;
  }
}

export default new AudioManager();
