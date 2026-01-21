import { useMemo } from 'react';
import { AudioProps, AudioPropsBase } from './types';

export function withPropsDefaults(props: AudioProps): AudioPropsBase {
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
  };
}

export function useStableAudioProps(props: AudioProps): AudioPropsBase {
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

    // Event Props
    onLoadStart,
    onLoad,
    onError,
    onProgress,
    onSeeked,
    onEnded,
    onPlay,
    onPause,
  } = withPropsDefaults(props);

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
