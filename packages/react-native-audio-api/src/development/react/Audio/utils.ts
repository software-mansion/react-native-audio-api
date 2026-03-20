import { useMemo, useRef } from 'react';
import { Platform } from 'react-native';
import AudioContext from '../../../core/AudioContext';
import type BaseAudioContext from '../../../core/BaseAudioContext';
import { AudioProps, AudioPropsBase } from './types';

/**
 * Merge props with defaults. `resolvedContext` must be stable when using the
 * implicit default (see `useStableAudioProps` — one `AudioContext` per hook
 * mount).
 */
export function withPropsDefaults(
  props: AudioProps,
  resolvedContext: BaseAudioContext | null
): AudioPropsBase {
  return {
    ...props,
    autoPlay: props.autoPlay ?? false,
    controls: props.controls ?? false,
    loop: props.loop ?? false,
    muted: props.muted ?? false,
    preload: props.preload ?? 'auto',
    source: props.source ?? [],
    playbackRate: props.playbackRate ?? 1.0,
    preservesPitch: props.preservesPitch ?? true,
    volume: props.volume ?? 1.0,
    context: resolvedContext,
  };
}

export function useStableAudioProps(props: AudioProps): AudioPropsBase {
  const defaultContextRef = useRef<BaseAudioContext | null>(null);
  const resolvedContext: BaseAudioContext | null =
    Platform.OS === 'web'
      ? null
      : (() => {
          if (defaultContextRef.current === null) {
            defaultContextRef.current = new AudioContext();
          }
          return props.context ?? defaultContextRef.current;
        })();

  const {
    // Control Props
    autoPlay,
    controls,
    loop,
    muted,
    preload,
    source,
    playbackRate,
    preservesPitch,
    volume,
    context,

    // Event Props
    onLoadStart,
    onLoad,
    onError,
    onProgress,
    onSeeked,
    onEnded,
    onPlay,
    onPause,
  } = withPropsDefaults(props, resolvedContext);

  return useMemo(
    () => ({
      // Control Props
      autoPlay,
      controls,
      loop,
      muted,
      preload,
      source,
      playbackRate,
      preservesPitch,
      volume,
      context,

      // Event Props
      onLoadStart,
      onLoad,
      onError,
      onProgress,
      onSeeked,
      onEnded,
      onPlay,
      onPause,
    }),
    [
      autoPlay,
      controls,
      loop,
      muted,
      preload,
      source,
      playbackRate,
      preservesPitch,
      volume,
      context,
      onLoadStart,
      onLoad,
      onError,
      onProgress,
      onSeeked,
      onEnded,
      onPlay,
      onPause,
    ]
  );
}
