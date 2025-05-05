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

interface RemoteEvents {
  remotePlay: RemoteEmptyType;
  remotePause: RemoteEmptyType;
  remoteStop: RemoteEmptyType;
  remoteTogglePlayPause: RemoteEmptyType;
  remoteChangePlaybackRate: RemoteEmptyType;
  remoteNextTrack: RemoteEmptyType;
  remotePreviousTrack: RemoteEmptyType;
  remoteSkipForward: RemoteEmptyType;
  remoteSkipBackward: RemoteEmptyType;
  remoteSeekForward: RemoteEmptyType;
  remoteSeekBackward: RemoteEmptyType;
  remoteChangePlaybackPosition: RemoteEmptyType;
  volumeChange: RemoteControlEventType;
  interruption: OnInterruptionEventType;
  routeChange: OnRouteChangeEventType;
}

export type RemoteEventName = keyof RemoteEvents;
export type RemoteEventCallback<Name extends RemoteEventName> = (
  event: RemoteEvents[Name]
) => void;

export type PermissionStatus = 'Undetermined' | 'Denied' | 'Granted';
