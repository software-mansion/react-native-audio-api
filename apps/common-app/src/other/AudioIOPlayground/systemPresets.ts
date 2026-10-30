import type {
  AndroidInputPreset,
  AndroidOutputProfile,
  AndroidSystemOptions,
  AudioContextOptions,
  AudioFocusType,
  AudioRecorderOptions,
  IOSCategory,
  IOSMode,
  IOSOption,
} from 'react-native-audio-api';

export interface IosSettings {
  category: IOSCategory;
  mode: IOSMode;
  options: IOSOption[];
}

/** An omitted field is what `setSystemOptions` receives when it is not set. */
export type AndroidSettings = AndroidSystemOptions;

/**
 * `observeAudioInterruptions(false)`: no focus request, other apps keep
 * playing.
 */
export const NO_FOCUS = 'none';
export type FocusChoice = AudioFocusType | typeof NO_FOCUS;

/**
 * What a preset sets on one platform. Besides the `setSystemOptions` group it
 * carries the recorder and context constructor options, and on Android the
 * focus `observeAudioInterruptions` requests. iOS ignores the focus type, so
 * its draft has none.
 */
export type IosDraft = IosSettings & { iosVoiceProcessing: boolean };
export type AndroidDraft = AndroidSettings & {
  focus: FocusChoice;
  androidInputPreset: AndroidInputPreset;
  androidOutputProfile: AndroidOutputProfile;
};

interface Preset {
  ios: IosDraft;
  android: AndroidDraft;
}

export const CUSTOM_PRESET = 'Custom';

const SPEAKER_WITH_BLUETOOTH: IOSOption[] = [
  'defaultToSpeaker',
  'allowBluetoothA2DP',
];

/**
 * One list for both platforms: each preset names a behaviour and spells it in
 * each platform's terms. Other apps' audio is the iOS category options on iOS
 * and the focus type on Android. Echo cancellation is voice processing with
 * `voiceChat` on iOS and the `voiceCommunication` input preset in
 * `inCommunication` mode on Android.
 */
export const PRESETS = {
  'Mix with others': {
    ios: {
      category: 'playAndRecord',
      mode: 'default',
      options: [...SPEAKER_WITH_BLUETOOTH, 'mixWithOthers'],
      iosVoiceProcessing: false,
    },
    android: {
      focus: NO_FOCUS,
      androidInputPreset: 'voiceRecognition',
      androidOutputProfile: 'media',
    },
  },
  'Duck others': {
    ios: {
      category: 'playAndRecord',
      mode: 'default',
      options: [...SPEAKER_WITH_BLUETOOTH, 'duckOthers'],
      iosVoiceProcessing: false,
    },
    android: {
      focus: 'gainTransientMayDuck',
      androidInputPreset: 'voiceRecognition',
      androidOutputProfile: 'media',
    },
  },
  'Interrupt others': {
    ios: {
      category: 'playAndRecord',
      mode: 'default',
      options: SPEAKER_WITH_BLUETOOTH,
      iosVoiceProcessing: false,
    },
    android: {
      focus: 'gain',
      androidInputPreset: 'voiceRecognition',
      androidOutputProfile: 'media',
    },
  },
  'Record demo': {
    ios: {
      category: 'playAndRecord',
      mode: 'default',
      options: [...SPEAKER_WITH_BLUETOOTH, 'mixWithOthers'],
      iosVoiceProcessing: false,
    },
    android: {
      focus: 'gain',
      androidInputPreset: 'voiceCommunication',
      androidOutputProfile: 'media',
    },
  },
  'Voice call, speaker': {
    ios: {
      category: 'playAndRecord',
      mode: 'voiceChat',
      options: ['defaultToSpeaker'],
      iosVoiceProcessing: true,
    },
    android: {
      mode: 'inCommunication',
      communicationDevice: 'speaker',
      focus: 'gain',
      androidInputPreset: 'voiceCommunication',
      androidOutputProfile: 'media',
    },
  },
  'Voice call, earpiece': {
    ios: {
      category: 'playAndRecord',
      mode: 'voiceChat',
      // Without defaultToSpeaker, playAndRecord routes to the receiver.
      options: [],
      iosVoiceProcessing: true,
    },
    android: {
      mode: 'inCommunication',
      communicationDevice: 'earpiece',
      focus: 'gain',
      androidInputPreset: 'voiceCommunication',
      androidOutputProfile: 'voiceCommunication',
    },
  },
} satisfies Record<string, Preset>;

export type PresetName = keyof typeof PRESETS;
export type PresetChoice = PresetName | typeof CUSTOM_PRESET;

export const DEFAULT_PRESET: PresetName = 'Record demo';
export const PRESET_NAMES = Object.keys(PRESETS) as PresetName[];

export function iosSystemGroup({
  category,
  mode,
  options,
}: IosDraft): IosSettings {
  return { category, mode, options };
}

export function androidSystemGroup({
  mode,
  communicationDevice,
}: AndroidDraft): AndroidSettings {
  return { mode, communicationDevice };
}

export function iosRecorderOptions(draft: IosDraft): AudioRecorderOptions {
  return { iosVoiceProcessing: draft.iosVoiceProcessing };
}

export function androidRecorderOptions(
  draft: AndroidDraft
): AudioRecorderOptions {
  return { androidInputPreset: draft.androidInputPreset };
}

export function androidContextOptions(
  draft: AndroidDraft
): AudioContextOptions {
  return { androidOutputProfile: draft.androidOutputProfile };
}
