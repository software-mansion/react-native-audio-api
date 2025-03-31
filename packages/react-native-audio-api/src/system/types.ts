export type IOSCategory =
  | 'record'
  | 'ambient'
  | 'playback'
  | 'multiRoute'
  | 'soloAmbient'
  | 'playbackAndRecord';

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
  active?: boolean;
}

export type MediaState = 'state_playing' | 'state_paused' | 'state_stopped';

interface BaseLockScreenInfo {
  [key: string]: string | boolean | number | undefined;
}

export interface LockScreenInfo extends BaseLockScreenInfo {
  title?: string;
  artwork?: string;
  artist?: string;
  album?: string;
  genre?: string;
  duration?: number;
  isLiveStream?: boolean;
  state?: MediaState;
  speed?: number;
  elapsedTime?: number;
}

export type RemoteControl =
  | 'play'
  | 'pause'
  | 'stop'
  | 'togglePlayPause'
  | 'changePlaybackRate'
  | 'nextTrack'
  | 'previousTrack'
  | 'skipForward'
  | 'skipBackward'
  | 'seekForward'
  | 'seekBackward'
  | 'changePlaybackPosition';
