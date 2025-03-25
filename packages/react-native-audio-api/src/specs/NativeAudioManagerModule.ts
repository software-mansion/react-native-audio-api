import { TurboModuleRegistry, TurboModule } from 'react-native';
import { EventEmitter } from 'react-native/Libraries/Types/CodegenTypes';

interface Spec extends TurboModule {
  setLockScreenInfo(info: {
    [key: string]: string | boolean | number | undefined;
  }): void;
  resetLockScreenInfo(): void;
  setSessionOptions(
    category: string,
    mode: string,
    options: Array<string>,
    active: boolean
  ): void;
  getDevicePreferredSampleRate(): number;

  readonly onRemotePlay: EventEmitter<void>;
  readonly onRemotePause: EventEmitter<void>;
  readonly onStop: EventEmitter<void>;
  readonly onTogglePlayPause: EventEmitter<void>;
  readonly onChangePlaybackRate: EventEmitter<number>;
  readonly onNextTrack: EventEmitter<void>;
  readonly onPreviousTrack: EventEmitter<void>;
  readonly onSkipForward: EventEmitter<number>;
  readonly onSkipBackward: EventEmitter<number>;
  readonly onSeekForward: EventEmitter<void>;
  readonly onSeekBackward: EventEmitter<void>;
  readonly onChangePlaybackPosition: EventEmitter<number>;
}

export default TurboModuleRegistry.getEnforcing<Spec>('AudioManagerModule');
