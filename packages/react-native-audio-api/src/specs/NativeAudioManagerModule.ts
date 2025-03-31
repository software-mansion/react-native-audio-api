import { TurboModuleRegistry, TurboModule } from 'react-native';
import { EventEmitter } from 'react-native/Libraries/Types/CodegenTypes';

interface Spec extends TurboModule {
  // lock screen
  setLockScreenInfo(info: {
    [key: string]: string | boolean | number | undefined;
  }): void;
  resetLockScreenInfo(): void;
  enableRemoteCommand(name: string, enabled: boolean): void;

  readonly onRemotePlay: EventEmitter<void>;
  readonly onRemotePause: EventEmitter<void>;
  readonly onRemoteStop: EventEmitter<void>;
  readonly onRemoteTogglePlayPause: EventEmitter<void>;
  readonly onRemoteChangePlaybackRate: EventEmitter<number>;
  readonly onRemoteNextTrack: EventEmitter<void>;
  readonly onRemotePreviousTrack: EventEmitter<void>;
  readonly onRemoteSkipForward: EventEmitter<number>;
  readonly onRemoteSkipBackward: EventEmitter<number>;
  readonly onRemoteSeekForward: EventEmitter<void>;
  readonly onRemoteSeekBackward: EventEmitter<void>;
  readonly onRemoteChangePlaybackPosition: EventEmitter<number>;

  // audio session
  setSessionOptions(
    category: string,
    mode: string,
    options: Array<string>,
    active: boolean
  ): void;
  getDevicePreferredSampleRate(): number;
}

export default TurboModuleRegistry.getEnforcing<Spec>('AudioManagerModule');
