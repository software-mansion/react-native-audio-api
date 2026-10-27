import type {
  AndroidSystemOptions,
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

export const CUSTOM_PRESET = 'Custom';

export const IOS_PRESETS = {
  'Record demo': {
    category: 'playAndRecord',
    mode: 'default',
    options: ['defaultToSpeaker', 'allowBluetoothA2DP', 'mixWithOthers'],
  },
  'Record demo, exclusive': {
    category: 'playAndRecord',
    mode: 'default',
    options: ['defaultToSpeaker', 'allowBluetoothA2DP'],
  },
  Playback: {
    category: 'playback',
    mode: 'default',
    options: [],
  },
} satisfies Record<string, IosSettings>;

export const ANDROID_PRESETS = {
  // No mode: Activate and Deactivate do nothing on Android.
  'Normal mode': {},
  'Communication, speaker': {
    mode: 'inCommunication',
    communicationDevice: 'speaker',
  },
  'Communication, earpiece': {
    mode: 'inCommunication',
    communicationDevice: 'earpiece',
  },
} satisfies Record<string, AndroidSettings>;

export type IosPresetName = keyof typeof IOS_PRESETS;
export type AndroidPresetName = keyof typeof ANDROID_PRESETS;

export const DEFAULT_IOS_PRESET: IosPresetName = 'Record demo';
export const DEFAULT_ANDROID_PRESET: AndroidPresetName = 'Normal mode';

export const IOS_PRESET_NAMES = Object.keys(IOS_PRESETS) as IosPresetName[];
export const ANDROID_PRESET_NAMES = Object.keys(
  ANDROID_PRESETS
) as AndroidPresetName[];

function isSameIosSettings(left: IosSettings, right: IosSettings): boolean {
  return (
    left.category === right.category &&
    left.mode === right.mode &&
    left.options.length === right.options.length &&
    left.options.every((option) => right.options.includes(option))
  );
}

function isSameAndroidSettings(
  left: AndroidSettings,
  right: AndroidSettings
): boolean {
  return (
    left.mode === right.mode &&
    left.communicationDevice === right.communicationDevice
  );
}

export function matchingIosPreset(
  settings: IosSettings
): IosPresetName | typeof CUSTOM_PRESET {
  const match = IOS_PRESET_NAMES.find((name) =>
    isSameIosSettings(IOS_PRESETS[name], settings)
  );
  return match ?? CUSTOM_PRESET;
}

export function matchingAndroidPreset(
  settings: AndroidSettings
): AndroidPresetName | typeof CUSTOM_PRESET {
  const match = ANDROID_PRESET_NAMES.find((name) =>
    isSameAndroidSettings(ANDROID_PRESETS[name], settings)
  );
  return match ?? CUSTOM_PRESET;
}
