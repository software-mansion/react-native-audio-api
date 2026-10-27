import { useCallback } from 'react';
import { Platform } from 'react-native';
import { AudioManager, SystemOptions } from 'react-native-audio-api';

import { describeCall, Log, logAction } from './log';

export const SYSTEM_CALLS = {
  apply: `AudioManager.setSystemOptions({ ${Platform.OS}: <fields above> })`,
  activate: 'AudioManager.setAudioSessionActivity(true)',
  deactivate: 'AudioManager.setAudioSessionActivity(false)',
};

export function useSystem(log: Log) {
  const apply = useCallback(
    (options: SystemOptions) =>
      logAction(
        log,
        describeCall('AudioManager.setSystemOptions', options),
        () => AudioManager.setSystemOptions(options)
      ),
    [log]
  );

  const activate = useCallback(
    () =>
      logAction(log, SYSTEM_CALLS.activate, () =>
        AudioManager.setAudioSessionActivity(true)
      ),
    [log]
  );

  const deactivate = useCallback(
    () =>
      logAction(log, SYSTEM_CALLS.deactivate, () =>
        AudioManager.setAudioSessionActivity(false)
      ),
    [log]
  );

  return { apply, activate, deactivate };
}
