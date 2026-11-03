import { useEffect } from 'react';
import { AppState } from 'react-native';
import { AudioManager } from 'react-native-audio-api';

import type { Log } from './log';

/** Each event line names the listener it came from and its payload as JSON. */
export function useSystemEventLog(log: Log) {
  useEffect(() => {
    // 'gain' is what `true` means; spelled out so the log names the focus.
    AudioManager.observeAudioInterruptions('gain');
    log(`AudioManager.observeAudioInterruptions('gain')`);

    const subscriptions = [
      AudioManager.addSystemEventListener('interruption', (event) =>
        log(`'interruption' event ${JSON.stringify(event)}`)
      ),
      AudioManager.addSystemEventListener('routeChange', (event) =>
        log(`'routeChange' event ${JSON.stringify(event)}`)
      ),
      AudioManager.addSystemEventListener('duck', () => log(`'duck' event`)),
      AppState.addEventListener('change', (state) =>
        log(`AppState 'change' event ${state}`)
      ),
    ];

    return () => {
      subscriptions.forEach((subscription) => subscription.remove());
      AudioManager.observeAudioInterruptions(false);
    };
  }, [log]);
}
