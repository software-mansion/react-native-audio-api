import { SessionOptions, LockScreenInfo } from './types';
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

  onRemotePlay(callback: () => void) {
    NativeAudioManagerModule.onRemotePlay(callback);
  }

  onRemotePause(callback: () => void) {
    NativeAudioManagerModule.onRemotePause(callback);
  }

  onRemoteStop(callback: () => void) {
    NativeAudioManagerModule.onRemoteStop(callback);
  }

  onRemoteTogglePlayPause(callback: () => void) {
    NativeAudioManagerModule.onRemoteTogglePlayPause(callback);
  }

  onRemoteChangePlaybackRate(callback: (rate: number) => void) {
    NativeAudioManagerModule.onRemoteChangePlaybackRate(callback);
  }

  onRemoteNextTrack(callback: () => void) {
    NativeAudioManagerModule.onRemoteNextTrack(callback);
  }

  onRemotePreviousTrack(callback: () => void) {
    NativeAudioManagerModule.onRemotePreviousTrack(callback);
  }

  onRemoteSkipForward(callback: (interval: number) => void) {
    NativeAudioManagerModule.onRemoteSkipForward(callback);
  }

  onRemoteSkipBackward(callback: (interval: number) => void) {
    NativeAudioManagerModule.onRemoteSkipBackward(callback);
  }

  onRemoteSeekForward(callback: () => void) {
    NativeAudioManagerModule.onRemoteSeekForward(callback);
  }

  onRemoteSeekBackward(callback: () => void) {
    NativeAudioManagerModule.onRemoteSeekBackward(callback);
  }

  onRemoteChangePlaybackPosition(callback: (position: number) => void) {
    NativeAudioManagerModule.onRemoteChangePlaybackPosition(callback);
  }
}

export default new AudioManager();
