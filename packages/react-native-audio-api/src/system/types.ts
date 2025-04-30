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

export interface SessionOptions {
  iosMode?: IOSMode;
  iosOptions?: IOSOption[];
  iosCategory?: IOSCategory;
}

export type MediaState = 'state_playing' | 'state_paused';

interface BaseLockScreenInfo {
  [key: string]: string | boolean | number | undefined;
}

export interface LockScreenInfo extends BaseLockScreenInfo {
  title?: string;
  artwork?: string;
  artist?: string;
  album?: string;
  duration?: number;
  description?: string; // android only
  state?: MediaState;
  speed?: number;
  elapsedTime?: number;
}

interface RemoteEmptyType {}

interface RemoteControlEventType {
  value: number;
}

interface OnInterruptionEventType {
  type: 'ended' | 'began';
  shouldResume: boolean;
}

interface OnRouteChangeEventType {
  reason: string;
}

interface RemoteCommands {
  play: RemoteEmptyType;
  pause: RemoteEmptyType;
  stop: RemoteEmptyType;
  togglePlayPause: RemoteEmptyType;
  changePlaybackRate: RemoteEmptyType;
  nextTrack: RemoteEmptyType;
  previousTrack: RemoteEmptyType;
  skipForward: RemoteEmptyType;
  skipBackward: RemoteEmptyType;
  seekForward: RemoteEmptyType;
  seekBackward: RemoteEmptyType;
  changePlaybackPosition: RemoteEmptyType;
}

interface RemoteEvents {
  volumeChange: RemoteControlEventType;
  interruption: OnInterruptionEventType;
  routeChange: OnRouteChangeEventType;
}

export type RemoteCommandName = keyof RemoteCommands;
export type RemoteEventName = keyof RemoteEvents;
export type RemoteEventCallback<Name extends RemoteEventName> = (
  event: RemoteEvents[Name]
) => void;

export type PermissionStatus = 'Undetermined' | 'Denied' | 'Granted';
