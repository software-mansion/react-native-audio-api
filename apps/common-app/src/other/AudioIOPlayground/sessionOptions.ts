import type { IOSCategory, IOSMode, IOSOption } from 'react-native-audio-api';

// Keyed records instead of arrays: a value added to the library's union fails
// to compile here until it is listed.
const CATEGORIES: Record<IOSCategory, true> = {
  ambient: true,
  multiRoute: true,
  playAndRecord: true,
  playback: true,
  record: true,
  soloAmbient: true,
};

const MODES: Record<IOSMode, true> = {
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

const OPTIONS: Record<IOSOption, true> = {
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

export const IOS_CATEGORIES = Object.keys(CATEGORIES) as IOSCategory[];
export const IOS_MODES = Object.keys(MODES) as IOSMode[];
export const IOS_OPTIONS = Object.keys(OPTIONS) as IOSOption[];

export interface SessionSettings {
  category: IOSCategory;
  mode: IOSMode;
  options: IOSOption[];
}

export const SESSION_PRESETS = {
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
} satisfies Record<string, SessionSettings>;

export type SessionPresetName = keyof typeof SESSION_PRESETS;

export const DEFAULT_PRESET: SessionPresetName = 'Record demo';
export const CUSTOM_PRESET = 'Custom';
export const PRESET_NAMES = Object.keys(SESSION_PRESETS) as SessionPresetName[];

function hasSameOptions(left: IOSOption[], right: IOSOption[]): boolean {
  return (
    left.length === right.length &&
    left.every((option) => right.includes(option))
  );
}

export function matchingPresetName(
  settings: SessionSettings
): SessionPresetName | typeof CUSTOM_PRESET {
  const match = PRESET_NAMES.find((name) => {
    const preset: SessionSettings = SESSION_PRESETS[name];
    return (
      preset.category === settings.category &&
      preset.mode === settings.mode &&
      hasSameOptions(preset.options, settings.options)
    );
  });
  return match ?? CUSTOM_PRESET;
}

export function describeSettings(settings: SessionSettings): string {
  const options = settings.options.length
    ? settings.options.join(', ')
    : 'no options';
  return `${settings.category} / ${settings.mode} / ${options}`;
}
