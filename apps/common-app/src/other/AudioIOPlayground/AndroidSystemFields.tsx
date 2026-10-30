import React, { FC } from 'react';
import type {
  AndroidAudioMode,
  AndroidInputPreset,
  AndroidOutputProfile,
  CommunicationDevice,
} from 'react-native-audio-api';

import {
  ANDROID_INPUT_PRESETS,
  ANDROID_MODES,
  ANDROID_OUTPUT_PROFILES,
  AUDIO_FOCUS_TYPES,
  COMMUNICATION_DEVICES,
} from '../../utils/systemOptions';
import { AndroidDraft, FocusChoice, NO_FOCUS } from './systemPresets';
import { ChoiceSelect, Field } from './ui';

/** The field is left out of `setSystemOptions`. */
const NOT_SET = 'not set';

type ModeChoice = AndroidAudioMode | typeof NOT_SET;
type DeviceChoice = CommunicationDevice | typeof NOT_SET;

const FOCUS_SHORT_LABELS: Record<FocusChoice, string> = {
  none: 'none',
  gain: 'gain',
  gainTransient: 'gainTr',
  gainTransientExclusive: 'excl',
  gainTransientMayDuck: 'mayDuck',
};

const MODE_SHORT_LABELS: Record<ModeChoice, string> = {
  'not set': 'unset',
  inCommunication: 'inComm',
};

const DEVICE_SHORT_LABELS: Record<DeviceChoice, string> = {
  'not set': 'unset',
  speaker: 'spk',
  earpiece: 'ear',
  systemDefault: 'sysDef',
};

const INPUT_PRESET_SHORT_LABELS: Record<AndroidInputPreset, string> = {
  generic: 'gen',
  camcorder: 'cam',
  voiceRecognition: 'vRec',
  voiceCommunication: 'vComm',
  unprocessed: 'unproc',
  voicePerformance: 'vPerf',
};

const OUTPUT_PROFILE_SHORT_LABELS: Record<AndroidOutputProfile, string> = {
  media: 'media',
  voiceCommunication: 'vComm',
};

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
  return (
    <>
      <Field label="Focus" shortLabel="Focus">
        <ChoiceSelect<FocusChoice>
          testID="system-focus-type"
          value={settings.focus}
          options={[NO_FOCUS, ...AUDIO_FOCUS_TYPES]}
          shortLabels={FOCUS_SHORT_LABELS}
          onChange={(focus) => onChange({ ...settings, focus })}
        />
      </Field>
      <Field label="Mode" shortLabel="Mode">
        <ChoiceSelect<ModeChoice>
          testID="system-mode"
          value={settings.mode ?? NOT_SET}
          options={[NOT_SET, ...ANDROID_MODES]}
          shortLabels={MODE_SHORT_LABELS}
          onChange={(mode) =>
            onChange({ ...settings, mode: fromChoice<AndroidAudioMode>(mode) })
          }
        />
      </Field>
      <Field label="Device" shortLabel="Dev">
        <ChoiceSelect<DeviceChoice>
          testID="system-communication-device"
          value={settings.communicationDevice ?? NOT_SET}
          options={[NOT_SET, ...COMMUNICATION_DEVICES]}
          shortLabels={DEVICE_SHORT_LABELS}
          onChange={(device) =>
            onChange({
              ...settings,
              communicationDevice: fromChoice<CommunicationDevice>(device),
            })
          }
        />
      </Field>
      <Field label="Input" shortLabel="In">
        <ChoiceSelect<AndroidInputPreset>
          testID="system-input-preset"
          value={settings.androidInputPreset}
          options={ANDROID_INPUT_PRESETS}
          shortLabels={INPUT_PRESET_SHORT_LABELS}
          onChange={(androidInputPreset) =>
            onChange({ ...settings, androidInputPreset })
          }
        />
      </Field>
      <Field label="Output" shortLabel="Out">
        <ChoiceSelect<AndroidOutputProfile>
          testID="system-output-profile"
          value={settings.androidOutputProfile}
          options={ANDROID_OUTPUT_PROFILES}
          shortLabels={OUTPUT_PROFILE_SHORT_LABELS}
          onChange={(androidOutputProfile) =>
            onChange({ ...settings, androidOutputProfile })
          }
        />
      </Field>
    </>
  );
};

export default AndroidSystemFields;
