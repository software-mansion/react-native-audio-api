import { SessionOptions, LockScreenInfo, RemoteControl } from './types';
import NativeAudioManagerModule from '../specs/NativeAudioManagerModule';

if (!NativeAudioManagerModule) {
  throw new Error(
    `Failed to install react-native-audio-api: The native module could not be found.`
  );
}

class AudioManager {
  setLockScreenInfo(info: LockScreenInfo) {
    NativeAudioManagerModule.setLockScreenInfo(info);
  }

  resetLockScreenInfo() {
    NativeAudioManagerModule.resetLockScreenInfo();
  }

  setSessionOptions(options: SessionOptions) {
    NativeAudioManagerModule.setSessionOptions(
      options.iosCategory || '',
      options.iosMode || '',
      options.iosOptions || [],
      options.active || true
    );
  }

  getDevicePreferredSampleRate(): number {
    return NativeAudioManagerModule.getDevicePreferredSampleRate();
  }

  enableRemoteCommand(
    name: RemoteControl,
    enabled: boolean,
    callback?: (value?: number) => void
  ) {
    NativeAudioManagerModule.enableRemoteCommand(name, enabled);

    if (enabled && callback) {
      switch (name) {
        case 'play':
          NativeAudioManagerModule.onRemotePlay(callback as () => void);
          break;

        case 'pause':
          NativeAudioManagerModule.onRemotePause(callback as () => void);
          break;

        case 'stop':
          NativeAudioManagerModule.onRemoteStop(callback as () => void);
          break;

        case 'togglePlayPause':
          NativeAudioManagerModule.onRemoteTogglePlayPause(
            callback as () => void
          );
          break;

        case 'changePlaybackRate':
          NativeAudioManagerModule.onRemoteChangePlaybackRate(
            callback as (rate: number) => void
          );
          break;

        case 'nextTrack':
          NativeAudioManagerModule.onRemoteNextTrack(callback as () => void);
          break;

        case 'previousTrack':
          NativeAudioManagerModule.onRemotePreviousTrack(
            callback as () => void
          );
          break;

        case 'skipForward':
          NativeAudioManagerModule.onRemoteSkipForward(
            callback as (interval: number) => void
          );
          break;

        case 'skipBackward':
          NativeAudioManagerModule.onRemoteSkipBackward(
            callback as (interval: number) => void
          );
          break;

        case 'seekForward':
          NativeAudioManagerModule.onRemoteSeekForward(callback as () => void);
          break;

        case 'seekBackward':
          NativeAudioManagerModule.onRemoteSeekBackward(callback as () => void);
          break;

        case 'changePlaybackPosition':
          NativeAudioManagerModule.onRemoteChangePlaybackPosition(
            callback as (position: number) => void
          );
          break;

        default:
          console.error('Unsupported RemoteControl action:', name);
      }
    }
  }
}

export default new AudioManager();
