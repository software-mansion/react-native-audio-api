import { useCallback } from 'react';
import { AudioManager } from 'react-native-audio-api';

import { Log, logAction } from './log';
import { describeSettings, SessionSettings } from './sessionOptions';

export function useSession(log: Log) {
  const apply = useCallback(
    (settings: SessionSettings) =>
      logAction(log, `session apply ${describeSettings(settings)}`, () =>
        AudioManager.setSystemOptions({
          ios: {
            category: settings.category,
            mode: settings.mode,
            options: settings.options,
          },
        })
      ),
    [log]
  );

  const activate = useCallback(
    () =>
      logAction(log, 'session activate', () =>
        AudioManager.setAudioSessionActivity(true)
      ),
    [log]
  );

  const deactivate = useCallback(
    () =>
      logAction(log, 'session deactivate', () =>
        AudioManager.setAudioSessionActivity(false)
      ),
    [log]
  );

  return { apply, activate, deactivate };
}
