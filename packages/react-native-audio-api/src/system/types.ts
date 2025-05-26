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

export type PermissionStatus = 'Undetermined' | 'Denied' | 'Granted';

type audioAttributeUsageType =
  | 'usage_alarm'
  | 'usage_assistance_accessibility'
  | 'usage_assistance_navigation_guidance'
  | 'usage_assistance_sonification'
  | 'usage_assistant'
  | 'usage_game'
  | 'usage_media'
  | 'usage_notification'
  | 'usage_notification_event'
  | 'usage_notification_ringtone'
  | 'usage_notification_communication_request'
  | 'usage_notification_communication_instant'
  | 'usage_notification_communication_delayed'
  | 'usage_unknown'
  | 'usage_voice_communication'
  | 'usage_voice_communication_signalling';

type audioAttributeContentType =
  | 'content_type_movie'
  | 'content_type_music'
  | 'content_type_sonification'
  | 'content_type_speech'
  | 'content_type_unknown';

type focusGainType =
  | 'audiofocus_gain'
  | 'audiofocus_gain_transient'
  | 'audiofocus_gain_transient_exclusive'
  | 'audiofocus_gain_transient_may_duck';

export type AudioAttributeType = {
  allowedCapturePolicy?:
    | 'allow_capture_by_all'
    | 'allow_capture_by_system'
    | 'allow_capture_by_none';
  contentType?: audioAttributeContentType;
  flag?: 'flag_hw_av_sync' | 'flag_audibility_enforced';
  hapticChannelsMuted?: boolean;
  isContentSpatialized?: boolean;
  spatializationBehavior?:
    | 'spatialization_behavior_auto'
    | 'spatialization_behavior_never';
  usage?: audioAttributeUsageType;
};

interface BaseRequestAudioFocusOptions {
  [key: string]: string | boolean | number | AudioAttributeType | undefined;
}

export interface RequestAudioFocusOptions extends BaseRequestAudioFocusOptions {
  acceptsDelayedFocusGain?: boolean;
  audioAttributes?: AudioAttributeType;
  focusGain?: focusGainType;
  pauseWhenDucked?: boolean;
}
