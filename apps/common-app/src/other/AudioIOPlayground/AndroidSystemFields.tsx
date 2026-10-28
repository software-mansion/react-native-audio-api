import React, { FC } from 'react';
import type {
  AndroidAudioMode,
  AndroidInputPreset,
  AndroidOutputProfile,
  CommunicationDevice,
} from 'react-native-audio-api';

import { Select } from '../../components';
import {
  ANDROID_INPUT_PRESETS,
  ANDROID_MODES,
  ANDROID_OUTPUT_PROFILES,
  COMMUNICATION_DEVICES,
} from '../../utils/systemOptions';
import {
  ANDROID_PRESET_NAMES,
  ANDROID_PRESETS,
  AndroidDraft,
  AndroidPresetName,
  CUSTOM_PRESET,
  matchingAndroidPreset,
} from './systemPresets';
import { Field } from './ui';

/** The field is left out of `setSystemOptions`. */
const NOT_SET = 'not set';

type PresetChoice = AndroidPresetName | typeof CUSTOM_PRESET;
type ModeChoice = AndroidAudioMode | typeof NOT_SET;
type DeviceChoice = CommunicationDevice | typeof NOT_SET;

function fromChoice<T extends string>(
  choice: T | typeof NOT_SET
): T | undefined {
  return choice === NOT_SET ? undefined : (choice as T);
}

interface AndroidSystemFieldsProps {
  settings: AndroidDraft;
  onChange: (settings: AndroidDraft) => void;
}

const AndroidSystemFields: FC<AndroidSystemFieldsProps> = ({
  settings,
  onChange,
}) => {
  const selectPreset = (name: PresetChoice) => {
    if (name !== CUSTOM_PRESET) {
      onChange(ANDROID_PRESETS[name]);
    }
  };

  return (
    <>
      <Field label="Preset">
        <Select<PresetChoice>
          testID="system-preset"
          value={matchingAndroidPreset(settings)}
          options={ANDROID_PRESET_NAMES}
          onChange={selectPreset}
        />
      </Field>
      <Field label="Mode">
        <Select<ModeChoice>
          testID="system-mode"
          value={settings.mode ?? NOT_SET}
          options={[NOT_SET, ...ANDROID_MODES]}
          onChange={(mode) =>
            onChange({ ...settings, mode: fromChoice<AndroidAudioMode>(mode) })
          }
        />
      </Field>
      <Field label="Device">
        <Select<DeviceChoice>
          testID="system-communication-device"
          value={settings.communicationDevice ?? NOT_SET}
          options={[NOT_SET, ...COMMUNICATION_DEVICES]}
          onChange={(device) =>
            onChange({
              ...settings,
              communicationDevice: fromChoice<CommunicationDevice>(device),
            })
          }
        />
      </Field>
      <Field label="Input">
        <Select<AndroidInputPreset>
          testID="system-input-preset"
          value={settings.androidInputPreset}
          options={ANDROID_INPUT_PRESETS}
          onChange={(androidInputPreset) =>
            onChange({ ...settings, androidInputPreset })
          }
        />
      </Field>
      <Field label="Output">
        <Select<AndroidOutputProfile>
          testID="system-output-profile"
          value={settings.androidOutputProfile}
          options={ANDROID_OUTPUT_PROFILES}
          onChange={(androidOutputProfile) =>
            onChange({ ...settings, androidOutputProfile })
          }
        />
      </Field>
    </>
  );
};

export default AndroidSystemFields;
