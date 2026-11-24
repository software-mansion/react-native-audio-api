import { useEffect, useState } from 'react';
import {
  AudioBuffer,
  AudioContext,
  AudioManager,
  BaseAudioContext,
} from 'react-native-audio-api';

type AudioContextOptions = ConstructorParameters<typeof AudioContext>[0];
type DecodableSource = Parameters<BaseAudioContext['decodeAudioData']>[0];

/** A context owned by one screen: created on first render, closed on unmount. */
export function useAudioContext(options?: AudioContextOptions): AudioContext {
  const [context] = useState(() => new AudioContext(options));

  useEffect(
    () => () => {
      context.close();
    },
    [context]
  );

  return context;
}

/**
 * Decodes `source` (a URL, a bundled `require`, or bytes) into `context`. Null
 * until it is ready; a decode that finishes after unmount is dropped.
 */
export function useDecodedAudio(
  context: BaseAudioContext,
  source: DecodableSource
): AudioBuffer | null {
  const [buffer, setBuffer] = useState<AudioBuffer | null>(null);

  useEffect(() => {
    let isCancelled = false;
    setBuffer(null);

    context
      .decodeAudioData(source)
      .then((decoded) => {
        if (!isCancelled) {
          setBuffer(decoded);
        }
      })
      .catch((error) => console.warn('Failed to decode audio', error));

    return () => {
      isCancelled = true;
    };
  }, [context, source]);

  return buffer;
}

/** Selects the iOS playback category and activates the session. */
export async function activatePlaybackSession() {
  AudioManager.setSystemOptions({
    ios: { category: 'playback', mode: 'default', options: [] },
  });
  await AudioManager.setAudioSessionActivity(true);
}
