import React, { FC, useState } from 'react';
import { Platform } from 'react-native';
import type {
  AudioContextOptions,
  AudioRecorderOptions,
} from 'react-native-audio-api';

import { Select } from '../../components';
import AndroidSystemFields from './AndroidSystemFields';
import IosSystemFields from './IosSystemFields';
import type { Log } from './log';
import {
  androidContextOptions,
  AndroidDraft,
  androidRecorderOptions,
  androidSystemGroup,
  CUSTOM_PRESET,
  DEFAULT_PRESET,
  IosDraft,
  iosRecorderOptions,
  iosSystemGroup,
  PRESET_NAMES,
  PresetChoice,
  PRESETS,
} from './systemPresets';
import { CallButton, Field, Section } from './ui';
import { PLAYBACK_CALLS } from './usePlayback';
import { RECORDER_CALLS } from './useRecorder';
import { SYSTEM_CALLS, useSystem } from './useSystem';

interface SystemPanelProps {
  log: Log;
  onRecreateRecorder: (options: AudioRecorderOptions) => void;
  onRecreateContext: (options: AudioContextOptions) => void;
}

/**
 * Shows only the current platform's fields. Apply sends only that platform's
 * `setSystemOptions` group: the other platform ignores it. The recorder and
 * context fields are constructor options, so they reach native only when the
 * matching button builds a new object. A preset fills both platforms' drafts;
 * editing a field turns the preset box to Custom.
 */
const SystemPanel: FC<SystemPanelProps> = ({
  log,
  onRecreateRecorder,
  onRecreateContext,
}) => {
  const [presetName, setPresetName] = useState<PresetChoice>(DEFAULT_PRESET);
  const [iosDraft, setIosDraft] = useState<IosDraft>(
    PRESETS[DEFAULT_PRESET].ios
  );
  const [androidDraft, setAndroidDraft] = useState<AndroidDraft>(
    PRESETS[DEFAULT_PRESET].android
  );
  const { apply, activate, deactivate, requestFocus } = useSystem(log);
  const isIos = Platform.OS === 'ios';

  const selectPreset = (name: PresetChoice) => {
    setPresetName(name);
    if (name !== CUSTOM_PRESET) {
      setIosDraft(PRESETS[name].ios);
      setAndroidDraft(PRESETS[name].android);
    }
  };

  const editIosDraft = (draft: IosDraft) => {
    setPresetName(CUSTOM_PRESET);
    setIosDraft(draft);
  };

  const editAndroidDraft = (draft: AndroidDraft) => {
    setPresetName(CUSTOM_PRESET);
    setAndroidDraft(draft);
  };

  return (
    <Section title="System">
      <Field label="Preset">
        <Select<PresetChoice>
          testID="system-preset"
          value={presetName}
          options={[...PRESET_NAMES, CUSTOM_PRESET]}
          onChange={selectPreset}
        />
      </Field>
      {isIos ? (
        <IosSystemFields settings={iosDraft} onChange={editIosDraft} />
      ) : (
        <AndroidSystemFields
          settings={androidDraft}
          onChange={editAndroidDraft}
        />
      )}
      {!isIos && (
        <CallButton
          testID="system-focus"
          title="Focus"
          call={SYSTEM_CALLS.focus}
          onPress={() => requestFocus(androidDraft.focus)}
        />
      )}
      <CallButton
        testID="system-apply"
        title="Apply"
        call={SYSTEM_CALLS.apply}
        onPress={() =>
          apply(
            isIos
              ? { ios: iosSystemGroup(iosDraft) }
              : { android: androidSystemGroup(androidDraft) }
          )
        }
      />
      <CallButton
        testID="system-activate"
        title="Activate"
        call={SYSTEM_CALLS.activate}
        onPress={activate}
      />
      <CallButton
        testID="system-deactivate"
        title="Deactivate"
        call={SYSTEM_CALLS.deactivate}
        onPress={deactivate}
      />
      <CallButton
        testID="system-recreate-recorder"
        title="New recorder"
        call={RECORDER_CALLS.recreate}
        onPress={() =>
          onRecreateRecorder(
            isIos
              ? iosRecorderOptions(iosDraft)
              : androidRecorderOptions(androidDraft)
          )
        }
      />
      <CallButton
        testID="system-recreate-context"
        title="New context"
        call={PLAYBACK_CALLS.recreate}
        onPress={() =>
          onRecreateContext(isIos ? {} : androidContextOptions(androidDraft))
        }
      />
    </Section>
  );
};

export default SystemPanel;
