import type {
  AndroidInputPreset,
  AndroidOutputProfile,
  AndroidSystemOptions,
  AudioContextOptions,
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
 * What a preset sets. Besides the `setSystemOptions` group it carries the
 * constructor options of the recorder and the context, which Recreate applies.
 * Only the current platform's fields are used.
 */
export type IosDraft = IosSettings & { iosVoiceProcessing: boolean };
export type AndroidDraft = AndroidSettings & {
  androidInputPreset: AndroidInputPreset;
  androidOutputProfile: AndroidOutputProfile;
};

export const CUSTOM_PRESET = 'Custom';

export const IOS_PRESETS = {
  'Record demo': {
    category: 'playAndRecord',
    mode: 'default',
    options: ['defaultToSpeaker', 'allowBluetoothA2DP', 'mixWithOthers'],
    iosVoiceProcessing: false,
  },
  'Record demo, exclusive': {
    category: 'playAndRecord',
    mode: 'default',
    options: ['defaultToSpeaker', 'allowBluetoothA2DP'],
    iosVoiceProcessing: false,
  },
  Playback: {
    category: 'playback',
    mode: 'default',
    options: [],
    iosVoiceProcessing: false,
  },
} satisfies Record<string, IosDraft>;

export const ANDROID_PRESETS = {
  // No mode: Activate and Deactivate do nothing on Android.
  'Normal mode': {
    androidInputPreset: 'voiceCommunication',
    androidOutputProfile: 'media',
  },
  'Normal mode, default mic': {
    androidInputPreset: 'voiceRecognition',
    androidOutputProfile: 'media',
  },
  'Communication, speaker': {
    mode: 'inCommunication',
    communicationDevice: 'speaker',
    androidInputPreset: 'voiceCommunication',
    androidOutputProfile: 'media',
  },
  'Communication, earpiece': {
    mode: 'inCommunication',
    communicationDevice: 'earpiece',
    androidInputPreset: 'voiceCommunication',
    androidOutputProfile: 'voiceCommunication',
  },
} satisfies Record<string, AndroidDraft>;

export type IosPresetName = keyof typeof IOS_PRESETS;
export type AndroidPresetName = keyof typeof ANDROID_PRESETS;

export const DEFAULT_IOS_PRESET: IosPresetName = 'Record demo';
export const DEFAULT_ANDROID_PRESET: AndroidPresetName = 'Normal mode';

export const IOS_PRESET_NAMES = Object.keys(IOS_PRESETS) as IosPresetName[];
export const ANDROID_PRESET_NAMES = Object.keys(
  ANDROID_PRESETS
) as AndroidPresetName[];

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

function isSameIosDraft(left: IosDraft, right: IosDraft): boolean {
  return (
    left.category === right.category &&
    left.mode === right.mode &&
    left.iosVoiceProcessing === right.iosVoiceProcessing &&
    left.options.length === right.options.length &&
    left.options.every((option) => right.options.includes(option))
  );
}

function isSameAndroidDraft(left: AndroidDraft, right: AndroidDraft): boolean {
  return (
    left.mode === right.mode &&
    left.communicationDevice === right.communicationDevice &&
    left.androidInputPreset === right.androidInputPreset &&
    left.androidOutputProfile === right.androidOutputProfile
  );
}

export function matchingIosPreset(
  draft: IosDraft
): IosPresetName | typeof CUSTOM_PRESET {
  const match = IOS_PRESET_NAMES.find((name) =>
    isSameIosDraft(IOS_PRESETS[name], draft)
  );
  return match ?? CUSTOM_PRESET;
}

export function matchingAndroidPreset(
  draft: AndroidDraft
): AndroidPresetName | typeof CUSTOM_PRESET {
  const match = ANDROID_PRESET_NAMES.find((name) =>
    isSameAndroidDraft(ANDROID_PRESETS[name], draft)
  );
  return match ?? CUSTOM_PRESET;
}
