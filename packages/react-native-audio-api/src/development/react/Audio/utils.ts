import { useMemo } from 'react';
import { Platform } from 'react-native';
import AudioContext from '../../../core/AudioContext';
import type BaseAudioContext from '../../../core/BaseAudioContext';
import { AudioProps, AudioPropsBase } from './types';

const noop = () => {};
const noopError = (_error: Error) => {};
const noopNumber = (_number: number) => {};

/**
 * Merge props with defaults. `resolvedContext` must be stable when using the
 * implicit default (see `useStableAudioProps` — one `AudioContext` per hook
 * mount).
 */
export function withPropsDefaults(
  props: AudioProps,
  resolvedContext: BaseAudioContext | undefined
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
    onLoadStart: props.onLoadStart ?? noop,
    onLoad: props.onLoad ?? noop,
    onError: props.onError ?? noopError,
    onPositionChange: props.onPositionChange ?? noopNumber,
    onEnded: props.onEnded ?? noop,
    onPlay: props.onPlay ?? noop,
    onPause: props.onPause ?? noop,
    onVolumeChange: props.onVolumeChange ?? noopNumber,
  };
}

export function useStableAudioProps(props: AudioProps): AudioPropsBase {
  const resolvedContext = useMemo(() => {
    if (Platform.OS === 'web') {
      return undefined;
    }
    return props.context ?? new AudioContext();
  }, [props.context]);

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
    onPositionChange,
    onEnded,
    onPlay,
    onPause,
    onVolumeChange,
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
      onPositionChange,
      onEnded,
      onPlay,
      onPause,
      onVolumeChange,
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
      onPositionChange,
      onEnded,
      onPlay,
      onPause,
      onVolumeChange,
    ]
  );
}
