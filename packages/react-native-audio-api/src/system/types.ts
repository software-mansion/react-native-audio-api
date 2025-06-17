// category that defines a set of audio behaviors, f.e. recording is only allowed in 'record' or 'playAndRecord' category
export type IOSCategory =
  | 'record'
  | 'ambient'
  | 'playback'
  | 'multiRoute'
  | 'soloAmbient'
  | 'playAndRecord';

// used to define specialized behaviors to an audio session
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
  | 'duckOthers' // reduce the volume of other audio sessions
  | 'allowAirPlay' // whether you can stream audio from this session to AirPlay devices
  | 'mixWithOthers' // mixing with other audio sessions
  | 'allowBluetooth' // deprecated, option that determines whether Bluetooth hands-free devices appear as available input routes
  | 'defaultToSpeaker' // whether audio from the session defaults to the built-in speaker instead of the receiver
  | 'allowBluetoothA2DP' // whether you can stream audio from this session to Bluetooth devices that support the Advanced Audio Distribution Profile
  | 'overrideMutedMicrophoneInterruption' // whether the system interrupts the audio session when it mutes the built-in microphone
  | 'interruptSpokenAudioAndMixWithOthers'; // whether to pause spoken audio from other apps when this session plays

// specifies why the source is playing and controls routing, focus, and volume decisions
export type audioAttributeUsageType =
  | 'usageAlarm'
  | 'usageAssistanceAccessibility'
  | 'usageAssistanceNavigationGuidance'
  | 'usageAssistanceSonification'
  | 'usageAssistant'
  | 'usageGame'
  | 'usageMedia'
  | 'usageNotification'
  | 'usageNotificationEvent'
  | 'usageNotificationRingtone'
  | 'usageNotificationCommunicationRequest'
  | 'usageNotificationCommunicationInstant'
  | 'usageNotificationCommunicationDelayed'
  | 'usageUnknown'
  | 'usageVoiceCommunication'
  | 'usageVoiceCommunicationSignalling';

// specifies what the source is playing
export type audioAttributeContentType =
  | 'contentTypeMovie'
  | 'contentTypeMusic'
  | 'contentTypeSonification'
  | 'contentTypeSpeech'
  | 'contentTypeUnknown';

export type focusGainType =
  | 'audiofocusGain' // indicate a gain of audio focus, or a request of audio focus, of unknown duration
  | 'audiofocusGainTransient' // indicate a temporary gain or request of audio focus, anticipated to last a short amount of time
  | 'audiofocusGainTransientExclusive' // indicate a temporary request of audio focus, anticipated to last a short amount of time, during which no other applications, or system components, should play anything
  | 'audiofocusGainTransientMayDuck'; // indicate a temporary request of audio focus, anticipated to last a short amount of time, and where it is acceptable for other audio applications to keep playing after having lowered their output level

export type AudioAttributeType = {
  allowedCapturePolicy?:
    | 'allowCaptureByAll' // audio may be captured by any app
    | 'allowCaptureBySystem' // audio may be captured by system apps only
    | 'allowCaptureByNone'; // audio may not be captured by any app
  contentType?: audioAttributeContentType;
  hapticChannelsMuted?: boolean;
  usage?: audioAttributeUsageType;
};

export type MediaState = 'statePlaying' | 'statePaused';

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

export type PermissionStatus = 'undetermined' | 'denied' | 'granted';

interface BaseAudioOptions {
  [key: string]:
    | string
    | string[]
    | boolean
    | number
    | AudioAttributeType
    | undefined;
}

export interface AudioOptions extends BaseAudioOptions {
  androidAcceptsDelayedFocusGain?: boolean; // marks this focus request as compatible with delayed focus
  androidAudioAttributes?: AudioAttributeType;
  androidFocusGain?: focusGainType;
  androidPauseWhenDucked?: boolean; // whether the app will pause when instructed to duck
  iosMode?: IOSMode;
  iosOptions?: IOSOption[];
  iosCategory?: IOSCategory;
}
