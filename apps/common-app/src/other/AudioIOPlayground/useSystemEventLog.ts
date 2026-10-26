import { useEffect } from 'react';
import { AppState } from 'react-native';
import { AudioManager } from 'react-native-audio-api';

import type { Log } from './log';

export function useSystemEventLog(log: Log) {
  useEffect(() => {
    AudioManager.observeAudioInterruptions(true);
    log('system observing interruptions');

    const subscriptions = [
      AudioManager.addSystemEventListener(
        'interruption',
        ({ type, shouldResume }) =>
          log(`system interruption ${type}, shouldResume ${shouldResume}`)
      ),
      AudioManager.addSystemEventListener('routeChange', ({ reason }) =>
        log(`system route change ${reason}`)
      ),
      AudioManager.addSystemEventListener('duck', () => log('system duck')),
      AppState.addEventListener('change', (state) => log(`app ${state}`)),
    ];

    return () => {
      subscriptions.forEach((subscription) => subscription.remove());
      AudioManager.observeAudioInterruptions(false);
    };
  }, [log]);
}
