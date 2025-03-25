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

  onStop(callback: () => void) {
    NativeAudioManagerModule.onStop(callback);
  }

  onTogglePlayPause(callback: () => void) {
    NativeAudioManagerModule.onTogglePlayPause(callback);
  }

  onChangePlaybackRate(callback: (rate: number) => void) {
    NativeAudioManagerModule.onChangePlaybackRate(callback);
  }

  onNextTrack(callback: () => void) {
    NativeAudioManagerModule.onNextTrack(callback);
  }

  onPreviousTrack(callback: () => void) {
    NativeAudioManagerModule.onPreviousTrack(callback);
  }

  onSkipForward(callback: (interval: number) => void) {
    NativeAudioManagerModule.onSkipForward(callback);
  }

  onSkipBackward(callback: (interval: number) => void) {
    NativeAudioManagerModule.onSkipBackward(callback);
  }

  onSeekForward(callback: () => void) {
    NativeAudioManagerModule.onSeekForward(callback);
  }

  onSeekBackward(callback: () => void) {
    NativeAudioManagerModule.onSeekBackward(callback);
  }

  onChangePlaybackPosition(callback: (position: number) => void) {
    NativeAudioManagerModule.onChangePlaybackPosition(callback);
  }
}

export default new AudioManager();
