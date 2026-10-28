import { useCallback, useEffect, useRef } from 'react';
import {
  AudioBufferSourceNode,
  AudioContext,
  AudioContextOptions,
} from 'react-native-audio-api';

import { describeCall, Log, logAction } from './log';
import { usePolled } from './usePolled';
import { useRecreatable } from './useRecreatable';

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

const CALLS = {
  suspend: 'context.suspend()',
  resume: 'context.resume()',
  stop: 'source.stop()',
  close: 'context.close()',
};

export const PLAYBACK_CALLS = {
  status: 'context.state, and whether a source is attached',
  play:
    'context.decodeAudioData(<source>), then context.createBufferSource(), ' +
    'source.loop = <loop>, source.connect(context.destination), source.start()',
  pause: CALLS.suspend,
  resume: CALLS.resume,
  stop: CALLS.stop,
  options: 'the options the current context was created with',
  recreate:
    'context = new AudioContext(<options above>); ' +
    `the previous context first releases its source and runs ${CALLS.close}`,
};

const DEFAULT_CONTEXT_OPTIONS: AudioContextOptions = {};

function createContext(options: AudioContextOptions): AudioContext {
  return new AudioContext(options);
}

function release(source: AudioBufferSourceNode) {
  source.onended = null;
  source.disconnect();
}

/**
 * Owns the playground's own context, so it never inherits a context another
 * screen left running. At most one source is attached at a time. The
 * context's options are fixed at construction, so `recreate` replaces it.
 */
export function usePlayback(log: Log, lastTakePath: string | null) {
  const {
    instance: context,
    options,
    recreate,
  } = useRecreatable(createContext, DEFAULT_CONTEXT_OPTIONS);
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
    log(describeCall('new AudioContext', options));
    context.onstatechange = () =>
      log(`context.onstatechange: ${context.state}`);
    context.onerror = () => log('context.onerror');

    return () => {
      if (sourceRef.current) {
        release(sourceRef.current);
        sourceRef.current = null;
      }
      logAction(log, CALLS.close, () => context.close());
    };
  }, [context, options, log]);

  const play = useCallback(
    async (sourceName: PlaybackSourceName, loop: boolean) => {
      const input =
        sourceName === LAST_TAKE ? lastTakePath : SAMPLE_TRACKS[sourceName];
      if (input === null) {
        log('play: no take recorded yet');
        return;
      }

      const decodeLabel =
        sourceName === LAST_TAKE
          ? describeCall('context.decodeAudioData', input)
          : `context.decodeAudioData(require(${sourceName}))`;
      const buffer = await logAction(log, decodeLabel, () =>
        context.decodeAudioData(input)
      );
      if (!buffer) {
        return;
      }

      const previous = sourceRef.current;
      if (previous) {
        await logAction(log, `${CALLS.stop} on the previous source`, () =>
          previous.stop()
        );
        release(previous);
      }

      await logAction(
        log,
        `context.createBufferSource(), source.loop = ${loop}, ` +
          'source.connect(context.destination), source.start()',
        () => {
          const source = context.createBufferSource();
          source.buffer = buffer;
          source.loop = loop;
          source.connect(context.destination);
          // The ref also keeps the node alive: once collected, its native ended
          // callback is cleared and onended never fires.
          sourceRef.current = source;
          source.onended = () => {
            log('source.onended');
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
    () => logAction(log, CALLS.suspend, () => context.suspend()),
    [context, log]
  );

  const resume = useCallback(
    () => logAction(log, CALLS.resume, () => context.resume()),
    [context, log]
  );

  const stop = useCallback(() => {
    const source = sourceRef.current;
    if (!source) {
      log(`${CALLS.stop}: no source`);
      return;
    }
    logAction(log, CALLS.stop, () => source.stop());
  }, [log]);

  return { status, options, play, pause, resume, stop, recreate };
}
