import type {
  AndroidAudioMode,
  AndroidInputPreset,
  AndroidOutputProfile,
  AudioFocusType,
  CommunicationDevice,
  IOSCategory,
  IOSMode,
  IOSOption,
} from 'react-native-audio-api';

// The library exports these unions only as types, which do not exist at
// runtime. Keyed records instead of arrays: a value added to a union fails to
// compile here until it is listed.

const IOS_CATEGORY_KEYS: Record<IOSCategory, true> = {
  ambient: true,
  multiRoute: true,
  playAndRecord: true,
  playback: true,
  record: true,
  soloAmbient: true,
};

const IOS_MODE_KEYS: Record<IOSMode, true> = {
  default: true,
  dualRoute: true,
  gameChat: true,
  measurement: true,
  moviePlayback: true,
  shortFormVideo: true,
  spokenAudio: true,
  videoChat: true,
  videoRecording: true,
  voiceChat: true,
  voicePrompt: true,
};

const IOS_OPTION_KEYS: Record<IOSOption, true> = {
  allowAirPlay: true,
  allowBluetoothA2DP: true,
  allowBluetoothHFP: true,
  bluetoothHighQualityRecording: true,
  defaultToSpeaker: true,
  duckOthers: true,
  farFieldInput: true,
  interruptSpokenAudioAndMixWithOthers: true,
  mixWithOthers: true,
  overrideMutedMicrophoneInterruption: true,
};

const ANDROID_MODE_KEYS: Record<AndroidAudioMode, true> = {
  inCommunication: true,
};

const COMMUNICATION_DEVICE_KEYS: Record<CommunicationDevice, true> = {
  speaker: true,
  earpiece: true,
  systemDefault: true,
};

const ANDROID_INPUT_PRESET_KEYS: Record<AndroidInputPreset, true> = {
  generic: true,
  camcorder: true,
  voiceRecognition: true,
  voiceCommunication: true,
  unprocessed: true,
  voicePerformance: true,
};

const ANDROID_OUTPUT_PROFILE_KEYS: Record<AndroidOutputProfile, true> = {
  media: true,
  voiceCommunication: true,
};

const AUDIO_FOCUS_TYPE_KEYS: Record<AudioFocusType, true> = {
  gain: true,
  gainTransient: true,
  gainTransientExclusive: true,
  gainTransientMayDuck: true,
};

export const IOS_CATEGORIES = Object.keys(IOS_CATEGORY_KEYS) as IOSCategory[];
export const IOS_MODES = Object.keys(IOS_MODE_KEYS) as IOSMode[];
export const IOS_OPTIONS = Object.keys(IOS_OPTION_KEYS) as IOSOption[];
export const ANDROID_MODES = Object.keys(
  ANDROID_MODE_KEYS
) as AndroidAudioMode[];
export const COMMUNICATION_DEVICES = Object.keys(
  COMMUNICATION_DEVICE_KEYS
) as CommunicationDevice[];
export const ANDROID_INPUT_PRESETS = Object.keys(
  ANDROID_INPUT_PRESET_KEYS
) as AndroidInputPreset[];
export const ANDROID_OUTPUT_PROFILES = Object.keys(
  ANDROID_OUTPUT_PROFILE_KEYS
) as AndroidOutputProfile[];
export const AUDIO_FOCUS_TYPES = Object.keys(
  AUDIO_FOCUS_TYPE_KEYS
) as AudioFocusType[];
