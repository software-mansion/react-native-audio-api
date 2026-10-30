import React, { FC } from 'react';
import type { IOSCategory, IOSMode, IOSOption } from 'react-native-audio-api';

import {
  IOS_CATEGORIES,
  IOS_MODES,
  IOS_OPTIONS,
} from '../../utils/systemOptions';
import type { IosDraft } from './systemPresets';
import { ChoiceSelect, Field, LabeledSwitch, MultiChoiceSelect } from './ui';

const CATEGORY_SHORT_LABELS: Record<IOSCategory, string> = {
  ambient: 'amb',
  multiRoute: 'multi',
  playAndRecord: 'P&R',
  playback: 'play',
  record: 'rec',
  soloAmbient: 'solo',
};

const MODE_SHORT_LABELS: Record<IOSMode, string> = {
  default: 'def',
  dualRoute: 'dual',
  gameChat: 'game',
  measurement: 'meas',
  moviePlayback: 'movie',
  shortFormVideo: 'short',
  spokenAudio: 'spoken',
  videoChat: 'vidChat',
  videoRecording: 'vidRec',
  voiceChat: 'vChat',
  voicePrompt: 'prompt',
};

const OPTION_SHORT_LABELS: Record<IOSOption, string> = {
  allowAirPlay: 'airPlay',
  allowBluetoothA2DP: 'btA2DP',
  allowBluetoothHFP: 'btHFP',
  bluetoothHighQualityRecording: 'btHQ',
  defaultToSpeaker: 'spk',
  duckOthers: 'duck',
  farFieldInput: 'far',
  interruptSpokenAudioAndMixWithOthers: 'intSpk',
  mixWithOthers: 'mix',
  overrideMutedMicrophoneInterruption: 'ovrMute',
};

interface IosSystemFieldsProps {
  settings: IosDraft;
  onChange: (settings: IosDraft) => void;
}

const IosSystemFields: FC<IosSystemFieldsProps> = ({ settings, onChange }) => {
  return (
    <>
      <Field label="Category" shortLabel="Cat">
        <ChoiceSelect
          testID="system-category"
          value={settings.category}
          options={IOS_CATEGORIES}
          shortLabels={CATEGORY_SHORT_LABELS}
          onChange={(category) => onChange({ ...settings, category })}
        />
      </Field>
      <Field label="Mode" shortLabel="Mode">
        <ChoiceSelect
          testID="system-mode"
          value={settings.mode}
          options={IOS_MODES}
          shortLabels={MODE_SHORT_LABELS}
          onChange={(mode) => onChange({ ...settings, mode })}
        />
      </Field>
      <Field label="Options" shortLabel="Opt">
        <MultiChoiceSelect
          testID="system-options"
          values={settings.options}
          options={IOS_OPTIONS}
          shortLabels={OPTION_SHORT_LABELS}
          onChange={(options) => onChange({ ...settings, options })}
        />
      </Field>
      <LabeledSwitch
        testID="system-voice-processing"
        label="Voice proc."
        shortLabel="VP"
        value={settings.iosVoiceProcessing}
        onValueChange={(iosVoiceProcessing) =>
          onChange({ ...settings, iosVoiceProcessing })
        }
      />
    </>
  );
};

export default IosSystemFields;
