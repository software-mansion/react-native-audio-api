import React, { FC, useState } from 'react';
import { Platform } from 'react-native';
import type {
  AudioContextOptions,
  AudioRecorderOptions,
} from 'react-native-audio-api';

import AndroidSystemFields from './AndroidSystemFields';
import IosSystemFields from './IosSystemFields';
import type { Log } from './log';
import { PLAYBACK_CALLS } from './usePlayback';
import { RECORDER_CALLS } from './useRecorder';
import {
  ANDROID_PRESETS,
  androidContextOptions,
  AndroidDraft,
  androidRecorderOptions,
  androidSystemGroup,
  DEFAULT_ANDROID_PRESET,
  DEFAULT_IOS_PRESET,
  IOS_PRESETS,
  IosDraft,
  iosRecorderOptions,
  iosSystemGroup,
} from './systemPresets';
import { CallButton, Section } from './ui';
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
 * matching Recreate button builds a new object.
 */
const SystemPanel: FC<SystemPanelProps> = ({
  log,
  onRecreateRecorder,
  onRecreateContext,
}) => {
  const [iosDraft, setIosDraft] = useState<IosDraft>(
    IOS_PRESETS[DEFAULT_IOS_PRESET]
  );
  const [androidDraft, setAndroidDraft] = useState<AndroidDraft>(
    ANDROID_PRESETS[DEFAULT_ANDROID_PRESET]
  );
  const { apply, activate, deactivate } = useSystem(log);
  const isIos = Platform.OS === 'ios';

  return (
    <Section title="System">
      {isIos ? (
        <IosSystemFields settings={iosDraft} onChange={setIosDraft} />
      ) : (
        <AndroidSystemFields
          settings={androidDraft}
          onChange={setAndroidDraft}
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
