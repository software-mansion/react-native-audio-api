import React, { FC, useState } from 'react';
import { Platform } from 'react-native';

import { Button } from '../../components';
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
import { ButtonRow, Section } from './ui';
import { useSystem } from './useSystem';

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
      <ButtonRow>
        <Button
          testID="system-apply"
          title="Apply"
          onPress={applyCurrentPlatform}
        />
        <Button testID="system-activate" title="Activate" onPress={activate} />
        <Button
          testID="system-deactivate"
          title="Deactivate"
          onPress={deactivate}
        />
      </ButtonRow>
    </Section>
  );
};

export default SystemPanel;
