import { useCallback, useEffect, useRef, useState } from 'react';
import { AudioBufferSourceNode, AudioContext } from 'react-native-audio-api';

import { Log, logAction } from './log';
import { usePolled } from './usePolled';

const SAMPLE_TRACKS = {
  'Music sample': require('../../demos/Crossfade/tracks/track1.mp3'),
  'Voice sample': require('../../examples/AudioFile/voice-sample-landing.mp3'),
};

export const LAST_TAKE = 'Last take';
export type PlaybackSourceName = typeof LAST_TAKE | keyof typeof SAMPLE_TRACKS;
export const PLAYBACK_SOURCE_NAMES: PlaybackSourceName[] = [
  LAST_TAKE,
  ...(Object.keys(SAMPLE_TRACKS) as (keyof typeof SAMPLE_TRACKS)[]),
];

function release(source: AudioBufferSourceNode) {
  source.onended = null;
  source.disconnect();
}

/**
 * Owns the playground's own context, so it never inherits a context another
 * screen left running. At most one source is attached at a time.
 */
export function usePlayback(log: Log, lastTakePath: string | null) {
  const [context] = useState(() => new AudioContext());
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);

  const status = usePolled(
    useCallback(() => {
      const source = sourceRef.current;
      const sourceState = source
        ? `source ${source.loop ? 'looping' : 'playing'}`
        : 'no source';
      return `context ${context.state}, ${sourceState}`;
    }, [context])
  );

  useEffect(() => {
    context.onstatechange = () => log(`playback context ${context.state}`);
    context.onerror = () => log('playback context error');

    return () => {
      if (sourceRef.current) {
        release(sourceRef.current);
      }
      logAction(log, 'playback close context', () => context.close());
    };
  }, [context, log]);

  const play = useCallback(
    async (sourceName: PlaybackSourceName, loop: boolean) => {
      const input =
        sourceName === LAST_TAKE ? lastTakePath : SAMPLE_TRACKS[sourceName];
      if (input === null) {
        log('playback play: no take recorded yet');
        return;
      }

      await logAction(
        log,
        `playback play ${sourceName}, loop ${loop}`,
        async () => {
          const buffer = await context.decodeAudioData(input);
          const previous = sourceRef.current;
          if (previous) {
            previous.stop();
            release(previous);
          }

          const source = context.createBufferSource();
          source.buffer = buffer;
          source.loop = loop;
          source.connect(context.destination);
          // The ref also keeps the node alive: once collected, its native ended
          // callback is cleared and onended never fires.
          sourceRef.current = source;
          source.onended = () => {
            log('playback source ended');
            release(source);
            if (sourceRef.current === source) {
              sourceRef.current = null;
            }
          };
          source.start();
        }
      );
    },
    [context, lastTakePath, log]
  );

  const pause = useCallback(
    () => logAction(log, 'playback suspend context', () => context.suspend()),
    [context, log]
  );

  const resume = useCallback(
    () => logAction(log, 'playback resume context', () => context.resume()),
    [context, log]
  );

  const stop = useCallback(() => {
    const source = sourceRef.current;
    if (!source) {
      log('playback stop: no source');
      return;
    }
    logAction(log, 'playback stop source', () => source.stop());
  }, [log]);

  return { status, play, pause, resume, stop };
}
