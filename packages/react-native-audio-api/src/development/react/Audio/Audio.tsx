import React, {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from 'react';
import { View, Image, Platform } from 'react-native';

import type {
  AudioProps,
  AudioTagPlaybackState,
  AudioURISource,
} from './types';
import { useStableAudioProps } from './utils';
import { AudioComponentContext } from './AudioTagContext';
import AudioControls from './controls/AudioControls';
import { AudioFileSourceNode } from './AudioFileSourceNode';
import { NotSupportedError } from '../../../errors';
import { NativeAudioAPIModule } from '../../../specs';

const Audio: React.FC<AudioProps> = (inProps) => {
  const { children } = inProps;

  /* eslint-disable @typescript-eslint/no-unused-vars */
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
  } = useStableAudioProps(inProps);

  const path = useMemo(() => {
    if (typeof source === 'string') {
      if (source.startsWith('file://') || source.startsWith('http')) {
        return source;
      }
      if (Platform.OS === 'android' && !__DEV__) {
        return NativeAudioAPIModule.resolveAndroidReleaseAsset(source);
      }
      return source;
    }
    // number
    if (typeof source === 'number') {
      return Image.resolveAssetSource(source).uri;
    }
    // AudioURISource
    return source.uri ?? '';
  }, [source]);

  const fileSourceRef = useRef<AudioFileSourceNode>(null);
  const sourceRef = useRef<ArrayBuffer | string | null>(null);
  const loopRef = useRef(loop);
  const effectiveVolumeRef = useRef(muted ? 0 : volume);
  const contextRef = useRef(context);

  const [volumeState, setVolumeState] = useState(volume);
  const [mutedState, setMutedState] = useState(muted);
  const [isReady, setIsReady] = useState(false);
  const [playbackState, setPlaybackState] =
    useState<AudioTagPlaybackState>('idle');
  const [currentTime, setCurrentTime] = useState(0);
  const [duration, setDuration] = useState(0);

  useEffect(() => {
    setVolumeState(volume);
    setMutedState(muted);
  }, [volume, muted]);

  useEffect(() => {
    effectiveVolumeRef.current = mutedState ? 0 : volumeState;
  }, [mutedState, volumeState]);

  const spawnFileSource = useCallback(() => {
    const ctx = contextRef.current;
    const src = sourceRef.current;
    if (!ctx || !src) {
      return;
    }

    const node = ctx.context.createFileSource({
      source: src,
      loop: loopRef.current,
      volume: effectiveVolumeRef.current,
    });
    if (!node) {
      throw new NotSupportedError('This file format requires FFmpeg build');
    }

    fileSourceRef.current = new AudioFileSourceNode(ctx, node);
    const { duration: d } = fileSourceRef.current.attach({
      loop: loopRef.current,
      onEnded: () => {
        setPlaybackState('idle');
        spawnFileSource();
      },
    });
    fileSourceRef.current.setVolume(effectiveVolumeRef.current);
    fileSourceRef.current.setLoop(loopRef.current);

    setDuration(d);
    setIsReady(true);
  }, [setDuration, setIsReady, setPlaybackState]);

  const play = useCallback(() => {
    if (!context) {
      return;
    }
    fileSourceRef.current?.play();
    setPlaybackState('playing');
  }, [context]);

  const pause = useCallback(() => {
    fileSourceRef.current?.pause();
    setPlaybackState('paused');
  }, []);

  const seekToTime = useCallback(
    (seconds: number) => {
      fileSourceRef.current?.seekToTime(seconds);
      const d = duration;
      const t =
        d > 0 ? Math.max(0, Math.min(seconds, d)) : Math.max(0, seconds);
      setCurrentTime(t);
    },
    [duration]
  );

  useEffect(() => {
    if (!path) {
      return;
    }

    const run = async () => {
      if (path.startsWith('http')) {
        await fetch(path, {
          headers: (source as AudioURISource).headers,
        })
          .then((response) => response.arrayBuffer())
          .then((arrayBuffer) => (sourceRef.current = arrayBuffer));
      } else {
        sourceRef.current = path;
      }
      spawnFileSource();
    };
    run();
  }, [path, spawnFileSource, source]);

  useEffect(() => {
    fileSourceRef.current?.setVolume(mutedState ? 0 : volumeState);
  }, [volumeState, mutedState]);

  useEffect(() => {
    fileSourceRef.current?.setLoop(loop);
  }, [loop]);

  useEffect(() => {
    if (playbackState !== 'playing') {
      return;
    }

    fileSourceRef.current?.startPositionTracking(setCurrentTime);

    return () => {
      fileSourceRef.current?.stopPositionTracking();
    };
  }, [playbackState]);

  const setVolume = useCallback((next: number) => {
    setVolumeState(next);
  }, []);

  const setMuted = useCallback((next: boolean) => {
    setMutedState(next);
  }, []);

  const ctxValue = useMemo(
    () => ({
      play,
      pause,
      seekToTime,
      volume: volumeState,
      setVolume,
      muted: mutedState,
      setMuted,
      isReady,
      playbackState,
      currentTime,
      duration,
    }),
    [
      play,
      pause,
      seekToTime,
      setVolume,
      volumeState,
      mutedState,
      setMuted,
      isReady,
      playbackState,
      currentTime,
      duration,
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
};

export default Audio;
