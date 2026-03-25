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
import AudioControls from './AudioControls';
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

  const fileSourceHandleRef = useRef(new AudioFileSourceNode());
  const loadedSourceRef = useRef<ArrayBuffer | string | null>(null);
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
  }, [volume]);

  useEffect(() => {
    setMutedState(muted);
  }, [muted]);

  useEffect(() => {
    effectiveVolumeRef.current = mutedState ? 0 : volumeState;
  }, [mutedState, volumeState]);

  const handlePlaybackEnded = useCallback(() => {
    setPlaybackState('idle');
    const ctx = contextRef.current;
    const src = loadedSourceRef.current;
    if (!ctx || !src) {
      return;
    }
    const handle = fileSourceHandleRef.current;
    const node = ctx.context.createFileSource(src);
    if (!node) {
      throw new NotSupportedError('This file format requires FFmpeg build');
    }
    const { duration: d } = handle.attach(node, {
      loop: loopRef.current,
      onEnded: () => handlePlaybackEnded(),
    });
    setDuration(d);
    handle.setVolume(effectiveVolumeRef.current);
    handle.setLoop(loopRef.current);
  }, []);

  const play = useCallback(() => {
    if (!context) {
      return;
    }
    fileSourceHandleRef.current.play(context);
    setPlaybackState('playing');
  }, [context]);

  const pause = useCallback(() => {
    fileSourceHandleRef.current.pause();
    setPlaybackState('paused');
  }, []);

  const seekToTime = useCallback(
    (seconds: number) => {
      fileSourceHandleRef.current.seekToTime(seconds);
      const d = duration;
      const t =
        d > 0 ? Math.max(0, Math.min(seconds, d)) : Math.max(0, seconds);
      setCurrentTime(t);
    },
    [duration]
  );

  const attachNode = useCallback(
    (sourceArg: ArrayBuffer | string) => {
      if (!context) {
        return;
      }
      loadedSourceRef.current = sourceArg;
      const handle = fileSourceHandleRef.current;
      const node = context.context.createFileSource(sourceArg);
      if (!node) {
        throw new NotSupportedError('This file format requires FFmpeg build');
      }
      const { duration: d } = handle.attach(node, {
        loop: loopRef.current,
        onEnded: () => {
          handlePlaybackEnded();
          setCurrentTime(d);
        },
      });
      setDuration(d);
      setIsReady(true);
      handle.setVolume(effectiveVolumeRef.current);
      handle.setLoop(loopRef.current);
      if (autoPlay) {
        play();
      }
    },
    [autoPlay, play, context, handlePlaybackEnded]
  );

  useEffect(() => {
    if (!context || !path) {
      return;
    }

    const fileSourceHandle = fileSourceHandleRef.current;

    const run = async () => {
      if (path.startsWith('http')) {
        const response = await fetch(path, {
          headers: (source as AudioURISource).headers,
        });
        const arrayBuffer = await response.arrayBuffer();
        attachNode(arrayBuffer);
      } else {
        attachNode(path);
      }
    };

    setIsReady(false);
    setPlaybackState('idle');
    run();

    return () => {
      fileSourceHandle.dispose();
      loadedSourceRef.current = null;
      setIsReady(false);
      setPlaybackState('idle');
    };
  }, [path, context, attachNode, source]);

  useEffect(() => {
    fileSourceHandleRef.current.setVolume(mutedState ? 0 : volumeState);
  }, [volumeState, mutedState]);

  useEffect(() => {
    fileSourceHandleRef.current.setLoop(loop);
  }, [loop]);

  useEffect(() => {
    if (playbackState !== 'playing') {
      return;
    }

    const handle = fileSourceHandleRef.current;
    handle.startPositionTracking(setCurrentTime);

    return () => {
      handle.stopPositionTracking();
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

  if (context == null) {
    return null;
  }

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
