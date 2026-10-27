import { useCallback } from 'react';
import { AudioManager, SystemOptions } from 'react-native-audio-api';

import { Log, logAction } from './log';
import { describeSystemOptions } from './systemPresets';

export function useSystem(log: Log) {
  const apply = useCallback(
    (options: SystemOptions) =>
      logAction(log, `system apply ${describeSystemOptions(options)}`, () =>
        AudioManager.setSystemOptions(options)
      ),
    [log]
  );

  const activate = useCallback(
    () =>
      logAction(log, 'system activate', () =>
        AudioManager.setAudioSessionActivity(true)
      ),
    [log]
  );

  const deactivate = useCallback(
    () =>
      logAction(log, 'system deactivate', () =>
        AudioManager.setAudioSessionActivity(false)
      ),
    [log]
  );

  return { apply, activate, deactivate };
}
