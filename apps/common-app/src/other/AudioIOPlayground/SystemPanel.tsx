import React, { FC, useState } from 'react';
import { Platform } from 'react-native';

import AndroidSystemFields from './AndroidSystemFields';
import IosSystemFields from './IosSystemFields';
import type { Log } from './log';
import {
  ANDROID_PRESETS,
  AndroidSettings,
  DEFAULT_ANDROID_PRESET,
  DEFAULT_IOS_PRESET,
  IOS_PRESETS,
  IosSettings,
} from './systemPresets';
import { CallButton, Section } from './ui';
import { SYSTEM_CALLS, useSystem } from './useSystem';

/**
 * Shows only the current platform's `setSystemOptions` group, and Apply sends
 * only that group: the other platform ignores it.
 */
const SystemPanel: FC<{ log: Log }> = ({ log }) => {
  const [iosSettings, setIosSettings] = useState<IosSettings>(
    IOS_PRESETS[DEFAULT_IOS_PRESET]
  );
  const [androidSettings, setAndroidSettings] = useState<AndroidSettings>(
    ANDROID_PRESETS[DEFAULT_ANDROID_PRESET]
  );
  const { apply, activate, deactivate } = useSystem(log);

  const applyCurrentPlatform = () =>
    apply(
      Platform.OS === 'ios'
        ? { ios: iosSettings }
        : { android: androidSettings }
    );

  return (
    <Section title="System">
      {Platform.OS === 'ios' ? (
        <IosSystemFields settings={iosSettings} onChange={setIosSettings} />
      ) : (
        <AndroidSystemFields
          settings={androidSettings}
          onChange={setAndroidSettings}
        />
      )}
      <CallButton
        testID="system-apply"
        title="Apply"
        call={SYSTEM_CALLS.apply}
        onPress={applyCurrentPlatform}
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
    </Section>
  );
};

export default SystemPanel;
