import React, {
  useCallback,
  useEffect,
  useImperativeHandle,
  useMemo,
  useRef,
  useState,
} from 'react';
import { View, Image, Platform } from 'react-native';

import type {
  AudioTagHandle,
  AudioProps,
  AudioTagPlaybackState,
  PreloadType,
} from './types';

import { AudioComponentContext } from './AudioTagContext';
import { AudioFileSourceNode } from './AudioFileSourceNode';
import { useStableAudioProps } from './utils';
import { NotSupportedError } from '../../../errors';
import { NativeAudioAPIModule } from '../../../specs';
import { AudioControls } from '..';
import { probeDuration } from '../../../core/AudioFileUtils';
import { base64ToArrayBuffer } from '../../../utils';
import { prefetchFileSegments } from './metadataPrefetching';

const Audio = React.forwardRef<AudioTagHandle, AudioProps>((props, ref) => {
  const { children } = props;
  const {
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
    onEnded: onEndedCallback,
    onPlay,
    onPause,
    onVolumeChange,
  } = useStableAudioProps(props);
  const audioContext = context ?? null;
  const [volumeState, setVolumeState] = useState<number | null>(null);
  const [mutedState, setMutedState] = useState<boolean | null>(null);
  const [ready, setReady] = useState(false);
  const [playbackState, setPlaybackState] =
    useState<AudioTagPlaybackState>('idle');
  const [currentTime, setCurrentTime] = useState(0);
  const [duration, setDuration] = useState(0);

  const path = useMemo(() => {
    if (!source) {
      return '';
    }
    if (typeof source === 'string') {
      return source;
    }
    // number
    if (typeof source === 'number') {
      return Image.resolveAssetSource(source).uri;
    }
    // AudioURISource
    return source.uri ?? '';
  }, [source]);

  const preloadMode: PreloadType =
    preload === 'none' || preload === 'metadata' ? preload : 'auto';
  const fileSourceRef = useRef<AudioFileSourceNode>(null);
  const fetchDataRef = useRef<(probe?: boolean) => Promise<void>>(
    async () => {}
  );
  const sourceRef = useRef<ArrayBuffer | string | null>(null);

  const isFetchingCancelled = useRef(false);
  const fullDataFetched = useRef(false);
  const lastEffectiveVolumeRef = useRef(muted ? 0 : volume);

  const effectiveMutedState = useMemo(() => {
    return mutedState ?? muted;
  }, [mutedState, muted]);

  const effectiveVolumeState = useMemo(() => {
    return effectiveMutedState ? 0 : (volumeState ?? volume);
  }, [effectiveMutedState, volumeState, volume]);

  const effectiveVolumeRef = useRef(effectiveVolumeState);
  effectiveVolumeRef.current = effectiveVolumeState;

  useEffect(() => {
    fileSourceRef.current?.setVolume(effectiveVolumeState);
  }, [effectiveVolumeState]);

  const play = useCallback(async () => {
    if (
      (preloadMode === 'none' || preloadMode === 'metadata') &&
      !fullDataFetched.current
    ) {
      await fetchDataRef.current(false);
    }
    fileSourceRef.current?.play();
    setPlaybackState('playing');
    onPlay();
  }, [onPlay, preloadMode]);

  const pause = useCallback(() => {
    fileSourceRef.current?.pause();
    setPlaybackState('paused');
    onPause();
  }, [onPause]);

  const seekToTime = useCallback(
    (seconds: number) => {
      fileSourceRef.current?.seekToTime(seconds);
      const nextTime =
        duration > 0
          ? Math.max(0, Math.min(seconds, duration))
          : Math.max(0, seconds);
      setCurrentTime(nextTime);
      onPositionChange(nextTime);
    },
    [duration, setCurrentTime, onPositionChange]
  );

  const spawnFileSource = useCallback(() => {
    const nextSource = sourceRef.current;
    if (!context || !nextSource) {
      return;
    }

    fileSourceRef.current?.dispose();
    setCurrentTime(0);
    setDuration(0);
    setPlaybackState('idle');

    const initialVolume = effectiveVolumeRef.current;

    const node = context.context.createFileSource({
      source: nextSource,
      loop,
      volume: initialVolume,
    });
    if (!node) {
      onError(new NotSupportedError('This file format requires FFmpeg build'));
      return;
    }

    const fileSource = new AudioFileSourceNode(context, node);
    const { duration: nextDuration } = fileSource.attach({
      loop,
      onEnded: () => {
        setPlaybackState('idle');
        setCurrentTime(nextDuration);
        onEndedCallback();
        spawnFileSource();
      },
    });

    fileSource.setVolume(initialVolume);
    fileSourceRef.current = fileSource;
    setDuration(nextDuration);
    onLoad();

    if (autoPlay) {
      play();
    }
  }, [context, loop, onError, onEndedCallback, onLoad, autoPlay, play]);

  const fetchData = useCallback(
    async (probe: boolean = false) => {
      isFetchingCancelled.current = false;
      setReady(false);
      onLoadStart();
      try {
        if (path.startsWith('http')) {
          if (
            preloadMode === 'metadata' &&
            probe &&
            ['opus', 'mp4', 'm4a', 'wav', 'flac'].some((extension) =>
              path.endsWith(extension)
            )
          ) {
            // fetch only metadata for codec that supports it
            const requestHeaders =
              typeof source === 'object' && source && 'headers' in source
                ? source.headers
                : undefined;
            const SEGMENT_SIZE = 1024 * 16;
            const prefetchedData = await prefetchFileSegments({
              url: path,
              headers: requestHeaders,
              startBytes: SEGMENT_SIZE,
              endBytes: SEGMENT_SIZE,
            });
            const probedDuration = await probeDuration(
              prefetchedData,
              context?.sampleRate
            );
            if (probedDuration != null && probedDuration > 0) {
              setDuration(probedDuration);
            }
            setReady(true);
            return;
          }
          const arrayBuffer = await fetch(path, {
            headers:
              typeof source === 'object' && source && 'headers' in source
                ? source.headers
                : undefined,
          }).then((response) => response.arrayBuffer());
          sourceRef.current = arrayBuffer;
        } else if (
          Platform.OS === 'android' &&
          !__DEV__ &&
          !path.startsWith('file://')
        ) {
          const base64Payload =
            await NativeAudioAPIModule.readAndroidReleaseAssetBytesAsBase64(
              path
            );
          const arrayBuffer = base64ToArrayBuffer(base64Payload);
          sourceRef.current = arrayBuffer;
        } else if (path.startsWith('file://')) {
          sourceRef.current = path.replace('file://', '');
        } else {
          sourceRef.current = path;
        }
        fullDataFetched.current = true;
        setReady(true);

        if (!isFetchingCancelled.current) {
          spawnFileSource();
          setReady(true);
        }
      } catch (error) {
        if (!isFetchingCancelled.current) {
          onError(error as Error);
        }
        setReady(false);
      }
    },
    [
      context?.sampleRate,
      onError,
      onLoadStart,
      path,
      preloadMode,
      source,
      spawnFileSource,
    ]
  );
  fetchDataRef.current = fetchData;

  useEffect(() => {
    isFetchingCancelled.current = false;
    fullDataFetched.current = false;

    if (!path) {
      setPlaybackState('idle');
      setCurrentTime(0);
      setDuration(0);
      fileSourceRef.current?.dispose();
      sourceRef.current = null;
      return;
    }

    if (preloadMode === 'none') {
      setReady(true);
      return;
    }

    if (preloadMode === 'metadata') {
      fetchData(true);
      return;
    }
    fetchData();

    return () => {
      isFetchingCancelled.current = true;
      fileSourceRef.current?.stopPositionTracking();
      fileSourceRef.current?.dispose();
    };
  }, [fetchData, path, preloadMode, source, spawnFileSource]);

  useEffect(() => {
    if (lastEffectiveVolumeRef.current !== effectiveVolumeState) {
      lastEffectiveVolumeRef.current = effectiveVolumeState;
      onVolumeChange(effectiveVolumeState);
    }
  }, [onVolumeChange, effectiveVolumeState]);

  useEffect(() => {
    fileSourceRef.current?.setLoop(loop);
  }, [loop]);

  useEffect(() => {
    if (playbackState !== 'playing') {
      return;
    }

    fileSourceRef.current?.startPositionTracking((seconds) => {
      setCurrentTime(seconds);
      onPositionChange(seconds);
    });

    return () => {
      fileSourceRef.current?.stopPositionTracking();
    };
  }, [onPositionChange, playbackState]);

  useImperativeHandle(
    ref,
    () => ({
      play,
      pause,
      seekToTime,
      setVolume: setVolumeState,
      setMuted: setMutedState,
    }),
    [pause, play, seekToTime, setMutedState, setVolumeState]
  );

  const ctxValue = useMemo(
    () => ({
      play,
      pause,
      seekToTime,
      setVolume: setVolumeState,
      volume: effectiveVolumeState,
      ready,
      setMuted: setMutedState,
      muted: effectiveMutedState,
      playbackState,
      currentTime,
      duration,
      autoPlay,
      controls,
      loop,
      preload,
      playbackRate,
      preservesPitch,
      sourcePath: path,
      source,
      audioContext,
    }),
    [
      play,
      pause,
      seekToTime,
      setVolumeState,
      effectiveVolumeState,
      ready,
      setMutedState,
      effectiveMutedState,
      playbackState,
      currentTime,
      duration,
      autoPlay,
      controls,
      loop,
      preload,
      playbackRate,
      preservesPitch,
      path,
      source,
      audioContext,
    ]
  );

  return (
    <AudioComponentContext.Provider value={ctxValue}>
      <View style={{ alignSelf: 'stretch', width: '100%' }}>
        {controls && <AudioControls />}
        {children}
      </View>
    </AudioComponentContext.Provider>
  );
});

export default Audio;
