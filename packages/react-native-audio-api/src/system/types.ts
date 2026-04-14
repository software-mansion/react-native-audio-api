import type { AudioEventSubscription } from '../events';
import type { SystemEventCallback, SystemEventName } from '../events/types';

export type IOSCategory =
  | 'ambient'
  | 'multiRoute'
  | 'playAndRecord'
  | 'playback'
  | 'record'
  | 'soloAmbient';

export type IOSMode =
  | 'default'
  | 'dualRoute'
  | 'gameChat'
  | 'measurement'
  | 'moviePlayback'
  | 'shortFormVideo'
  | 'spokenAudio'
  | 'videoChat'
  | 'videoRecording'
  | 'voiceChat'
  | 'voicePrompt';

export type IOSOption =
  | 'allowAirPlay'
  | 'allowBluetoothA2DP'
  | 'allowBluetoothHFP'
  | 'bluetoothHighQualityRecording'
  | 'defaultToSpeaker'
  | 'duckOthers'
  | 'farFieldInput'
  | 'interruptSpokenAudioAndMixWithOthers'
  | 'mixWithOthers'
  | 'overrideMutedMicrophoneInterruption';

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
  id: string;
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
  setInputDevice(deviceId: string): Promise<boolean>;
}
