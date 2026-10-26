import React, { FC, useState } from 'react';

import { Button, Select } from '../../components';
import type { Log } from './log';
import MultiSelect from './MultiSelect';
import {
  CUSTOM_PRESET,
  DEFAULT_PRESET,
  IOS_CATEGORIES,
  IOS_MODES,
  IOS_OPTIONS,
  matchingPresetName,
  PRESET_NAMES,
  SESSION_PRESETS,
  SessionPresetName,
  SessionSettings,
} from './sessionOptions';
import { ButtonRow, Field, Section } from './ui';
import { useSession } from './useSession';

type PresetChoice = SessionPresetName | typeof CUSTOM_PRESET;

const SessionPanel: FC<{ log: Log }> = ({ log }) => {
  const [settings, setSettings] = useState<SessionSettings>(
    SESSION_PRESETS[DEFAULT_PRESET]
  );
  const { apply, activate, deactivate } = useSession(log);

  const selectPreset = (name: PresetChoice) => {
    if (name !== CUSTOM_PRESET) {
      setSettings(SESSION_PRESETS[name]);
    }
  };

  return (
    <Section title="Session">
      <Field label="Preset">
        <Select<PresetChoice>
          testID="session-preset"
          value={matchingPresetName(settings)}
          options={PRESET_NAMES}
          onChange={selectPreset}
        />
      </Field>
      <Field label="Category">
        <Select
          testID="session-category"
          value={settings.category}
          options={IOS_CATEGORIES}
          onChange={(category) => setSettings({ ...settings, category })}
        />
      </Field>
      <Field label="Mode">
        <Select
          testID="session-mode"
          value={settings.mode}
          options={IOS_MODES}
          onChange={(mode) => setSettings({ ...settings, mode })}
        />
      </Field>
      <Field label="Options">
        <MultiSelect
          testID="session-options"
          values={settings.options}
          options={IOS_OPTIONS}
          onChange={(options) => setSettings({ ...settings, options })}
        />
      </Field>
      <ButtonRow>
        <Button
          testID="session-apply"
          title="Apply"
          onPress={() => apply(settings)}
        />
        <Button testID="session-activate" title="Activate" onPress={activate} />
        <Button
          testID="session-deactivate"
          title="Deactivate"
          onPress={deactivate}
        />
      </ButtonRow>
    </Section>
  );
};

export default SessionPanel;
