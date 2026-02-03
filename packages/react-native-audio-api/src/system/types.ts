import type { AudioEventSubscription } from '../events';
import type { SystemEventCallback, SystemEventName } from '../events/types';

export type IOSCategory =
  | 'record'
  | 'ambient'
  | 'playback'
  | 'multiRoute'
  | 'soloAmbient'
  | 'playAndRecord';

export type IOSMode =
  | 'default'
  | 'gameChat'
  | 'videoChat'
  | 'voiceChat'
  | 'measurement'
  | 'voicePrompt'
  | 'spokenAudio'
  | 'moviePlayback'
  | 'videoRecording';

export type IOSOption =
  | 'duckOthers'
  | 'allowAirPlay'
  | 'mixWithOthers'
  | 'allowBluetooth'
  | 'defaultToSpeaker'
  | 'allowBluetoothA2DP'
  | 'overrideMutedMicrophoneInterruption'
  | 'interruptSpokenAudioAndMixWithOthers';

export type AudioFocusType =
  | 'gain'
  | 'gainTransient'
  | 'gainTransientExclusive'
  | 'gainTransientMayDuck';

export interface SessionOptions {
  iosMode?: IOSMode;
  iosOptions?: IOSOption[];
  iosCategory?: IOSCategory;
  iosAllowHaptics?: boolean;
  /**
   * Setting this to `true` will allow other audio apps to resume playing when
   * the session is deactivated.
   *
   * Has no effect when using PlaybackNotificationManager as it takes over the
   * "Now playing" controls.
   */
  iosNotifyOthersOnDeactivation?: boolean;
}

export type PermissionStatus = 'Undetermined' | 'Denied' | 'Granted';

export interface AudioDeviceInfo {
  name: string;
  category: string;
}

export type AudioDeviceList = AudioDeviceInfo[];

export interface AudioDevicesInfo {
  availableInputs: AudioDeviceList;
  availableOutputs: AudioDeviceList;
  currentInputs: AudioDeviceList; // iOS only
  currentOutputs: AudioDeviceList; // iOS only
}

export interface IAudioManager {
  getDevicePreferredSampleRate(): number;
  setAudioSessionActivity(enabled: boolean): Promise<boolean>;
  setAudioSessionOptions(options: SessionOptions): void;
  disableSessionManagement(): void;
  observeAudioInterruptions(enabled: boolean): void;
  activelyReclaimSession(enabled: boolean): void;
  observeAudioInterruptions(param: AudioFocusType | boolean | null): void;
  addSystemEventListener<Name extends SystemEventName>(
    name: Name,
    callback: SystemEventCallback<Name>
  ): AudioEventSubscription | undefined;
  requestRecordingPermissions(): Promise<PermissionStatus>;
  checkRecordingPermissions(): Promise<PermissionStatus>;
  requestNotificationPermissions(): Promise<PermissionStatus>;
  checkNotificationPermissions(): Promise<PermissionStatus>;
  getDevicesInfo(): Promise<AudioDevicesInfo>;
}
