import React, { FC } from 'react';

import { Select } from '../../components';
import {
  IOS_CATEGORIES,
  IOS_MODES,
  IOS_OPTIONS,
} from '../../utils/systemOptions';
import MultiSelect from './MultiSelect';
import type { IosDraft } from './systemPresets';
import { Field, LabeledSwitch } from './ui';

interface IosSystemFieldsProps {
  settings: IosDraft;
  onChange: (settings: IosDraft) => void;
}

const IosSystemFields: FC<IosSystemFieldsProps> = ({ settings, onChange }) => {
  return (
    <>
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
      <LabeledSwitch
        testID="system-voice-processing"
        label="Voice proc."
        value={settings.iosVoiceProcessing}
        onValueChange={(iosVoiceProcessing) =>
          onChange({ ...settings, iosVoiceProcessing })
        }
      />
    </>
  );
};

export default IosSystemFields;
