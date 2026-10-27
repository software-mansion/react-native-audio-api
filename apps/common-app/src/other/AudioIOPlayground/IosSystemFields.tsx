import React, { FC } from 'react';

import { Select } from '../../components';
import {
  IOS_CATEGORIES,
  IOS_MODES,
  IOS_OPTIONS,
} from '../../utils/systemOptions';
import MultiSelect from './MultiSelect';
import {
  CUSTOM_PRESET,
  IOS_PRESET_NAMES,
  IOS_PRESETS,
  IosPresetName,
  IosSettings,
  matchingIosPreset,
} from './systemPresets';
import { Field } from './ui';

type PresetChoice = IosPresetName | typeof CUSTOM_PRESET;

interface IosSystemFieldsProps {
  settings: IosSettings;
  onChange: (settings: IosSettings) => void;
}

const IosSystemFields: FC<IosSystemFieldsProps> = ({ settings, onChange }) => {
  const selectPreset = (name: PresetChoice) => {
    if (name !== CUSTOM_PRESET) {
      onChange(IOS_PRESETS[name]);
    }
  };

  return (
    <>
      <Field label="Preset">
        <Select<PresetChoice>
          testID="system-preset"
          value={matchingIosPreset(settings)}
          options={IOS_PRESET_NAMES}
          onChange={selectPreset}
        />
      </Field>
      <Field label="Category">
        <Select
          testID="system-category"
          value={settings.category}
          options={IOS_CATEGORIES}
          onChange={(category) => onChange({ ...settings, category })}
        />
      </Field>
      <Field label="Mode">
        <Select
          testID="system-mode"
          value={settings.mode}
          options={IOS_MODES}
          onChange={(mode) => onChange({ ...settings, mode })}
        />
      </Field>
      <Field label="Options">
        <MultiSelect
          testID="system-options"
          values={settings.options}
          options={IOS_OPTIONS}
          onChange={(options) => onChange({ ...settings, options })}
        />
      </Field>
    </>
  );
};

export default IosSystemFields;
