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

export type AndroidAudioMode = 'inCommunication';

export type CommunicationDevice = 'speaker' | 'earpiece' | 'systemDefault';

/** AVAudioSession configuration; ignored on Android. */
export interface IOSSystemOptions {
  category?: IOSCategory;
  mode?: IOSMode;
  options?: IOSOption[];
  allowHaptics?: boolean;
  /**
   * Setting this to `true` will allow other audio apps to resume playing when
   * the session is deactivated.
   *
   * Has no effect when using PlaybackNotificationManager as it takes over the
   * "Now playing" controls.
   */
  notifyOthersOnDeactivation?: boolean;
}

/** Android audio-mode configuration; ignored on iOS. */
export interface AndroidSystemOptions {
  /**
   * Opts into Android's communication audio mode. Android 12 (API 31) or later
   * is required when this option is used.
   */
  mode?: AndroidAudioMode;
  /**
   * Initial communication-device preference. `systemDefault` clears an explicit
   * preference and lets Android choose the route.
   */
  communicationDevice?: CommunicationDevice;
}

export interface SystemOptions {
  ios?: IOSSystemOptions;
  android?: AndroidSystemOptions;
}

export type PermissionStatus = 'Undetermined' | 'Denied' | 'Granted';

export interface AudioDeviceInfo {
  id: string;
  name: string;
  category: string;
}

export type AudioDeviceList = AudioDeviceInfo[];

/**
 * An id from `AudioDevicesInfo.availableInputs`, or `'default'` to let the
 * system choose.
 */
export type InputDeviceSelection = 'default' | (string & {});

export interface AudioDevicesInfo {
  availableInputs: AudioDeviceList;
  availableOutputs: AudioDeviceList;
  /**
   * On iOS, the inputs of the current route. On Android, the device selected
   * through `setInputDevice`, and empty until one is selected: the platform
   * does not report which input it would pick on its own.
   */
  currentInputs: AudioDeviceList;
  /** Outputs of the current route. Always empty on Android. */
  currentOutputs: AudioDeviceList;
}

export interface IAudioManager {
  getDevicePreferredSampleRate(): number;
  setAudioSessionActivity(enabled: boolean): Promise<void>;
  setSystemOptions(options: SystemOptions): void;
  disableSessionManagement(): void;
  observeAudioInterruptions(enabled: boolean): void;
  activelyReclaimSession(enabled: boolean): void;
  observeAudioInterruptions(param: AudioFocusType | boolean | null): void;
  getSystemVolume(): number;
  addSystemEventListener<Name extends SystemEventName>(
    name: Name,
    callback: SystemEventCallback<Name>
  ): AudioEventSubscription | undefined;
  requestRecordingPermissions(): Promise<PermissionStatus>;
  checkRecordingPermissions(): Promise<PermissionStatus>;
  requestNotificationPermissions(): Promise<PermissionStatus>;
  checkNotificationPermissions(): Promise<PermissionStatus>;
  getDevicesInfo(): Promise<AudioDevicesInfo>;
  setInputDevice(deviceId: InputDeviceSelection): Promise<void>;
  setCommunicationDevice(device: CommunicationDevice): Promise<void>;
  getCommunicationDevice(): Promise<AudioDeviceInfo | null>;
}
