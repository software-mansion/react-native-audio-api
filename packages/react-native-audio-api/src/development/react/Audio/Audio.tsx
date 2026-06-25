import React, {
  useCallback,
  useEffect,
  useImperativeHandle,
  useMemo,
  useRef,
  useState,
} from 'react';
import { View } from 'react-native';

import type {
  AudioTagHandle,
  AudioProps,
  AudioTagPlaybackState,
} from './types';

import { AudioComponentContext } from './AudioTagContext';
import { useStableAudioProps, resolveSourcePath } from './utils';
import { AudioControls } from '..';
import { useAudioSourceLoader } from './useAudioSourceLoader';

const Audio = React.memo(
  React.forwardRef<AudioTagHandle, AudioProps>((props, ref) => {
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
    const [volumeState, setVolumeState] = useState<number | null>(null);
    const [mutedState, setMutedState] = useState<boolean | null>(null);
    const [ready, setReady] = useState(false);
    const [playbackState, setPlaybackState] =
      useState<AudioTagPlaybackState>('idle');
    const [currentTime, setCurrentTime] = useState(0);

    const path = useMemo(() => resolveSourcePath(source), [source]);

    const lastEffectiveVolumeRef = useRef(muted ? 0 : volume);

    useEffect(() => {
      return () => {
        fileSourceRef.current?.disconnect();
        fileSourceRef.current?.pause();
        fileSourceRef.current?.dispose();
        fileSourceRef.current = null;
      };
    }, []); // eslint-disable-line react-hooks/exhaustive-deps

    const effectiveMutedState = useMemo(() => {
      return mutedState ?? muted;
    }, [mutedState, muted]);

    const effectiveVolumeState = useMemo(() => {
      return effectiveMutedState ? 0 : (volumeState ?? volume);
    }, [effectiveMutedState, volumeState, volume]);

    const effectiveVolumeRef = useRef(effectiveVolumeState);
    effectiveVolumeRef.current = effectiveVolumeState;

    const playRef = useRef<() => void>(() => {});
    const handleAutoPlay = useCallback(() => {
      playRef.current();
    }, []);
    const handleEnded = useCallback(
      (endedDuration: number) => {
        setPlaybackState('idle');
        setCurrentTime(endedDuration);
        onEndedCallback();
      },
      [onEndedCallback]
    );

    const {
      fileSourceRef,
      ready: loaderReady,
      duration,
      loadForPlayback,
    } = useAudioSourceLoader({
      context,
      path,
      source,
      preloadMode: preload,
      loop,
      playbackRate,
      preservesPitch,
      autoPlay,
      effectiveVolumeRef,
      onLoadStart,
      onLoad,
      onError,
      onEnded: handleEnded,
      onAutoPlay: handleAutoPlay,
    });

    useEffect(() => {
      setReady(loaderReady);
    }, [loaderReady]);

    useEffect(() => {
      fileSourceRef.current?.setVolume(effectiveVolumeState);
    }, [effectiveVolumeState, fileSourceRef]);

    const play = useCallback(async () => {
      if (!fileSourceRef.current) {
        const loaded = await loadForPlayback();
        if (!loaded) {
          return;
        }
      }

      if (!fileSourceRef.current) {
        return;
      }

      fileSourceRef.current.play();
      setPlaybackState('playing');
      onPlay();
    }, [fileSourceRef, loadForPlayback, onPlay]);

    playRef.current = () => {
      play();
    };

    const pause = useCallback(() => {
      fileSourceRef.current?.pause();
      setPlaybackState('paused');
      onPause();
    }, [onPause, fileSourceRef]);

    const setPlaybackRate = useCallback(
      (nextPlaybackRate: number) => {
        fileSourceRef.current?.setPlaybackRate(nextPlaybackRate);
      },
      [fileSourceRef]
    );

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
      [duration, setCurrentTime, onPositionChange, fileSourceRef]
    );

    useEffect(() => {
      if (!path) {
        setPlaybackState('idle');
        setCurrentTime(0);
      }
    }, [path]);

    useEffect(() => {
      if (lastEffectiveVolumeRef.current !== effectiveVolumeState) {
        lastEffectiveVolumeRef.current = effectiveVolumeState;
        onVolumeChange(effectiveVolumeState);
      }
    }, [onVolumeChange, effectiveVolumeState]);

    useEffect(() => {
      fileSourceRef.current?.setLoop(loop);
    }, [loop, fileSourceRef]);

    useEffect(() => {
      fileSourceRef.current?.setPlaybackRate(playbackRate);
    }, [playbackRate, fileSourceRef]);

    useEffect(() => {
      fileSourceRef.current?.setPreservesPitch(preservesPitch);
    }, [preservesPitch, fileSourceRef]);

    useEffect(() => {
      if (playbackState !== 'playing') {
        return;
      }

      const fileSource = fileSourceRef.current;
      fileSource?.startPositionTracking((seconds) => {
        setCurrentTime(seconds);
        onPositionChange(seconds);
      });

      return () => {
        fileSource?.stopPositionTracking();
      };
    }, [onPositionChange, playbackState, fileSourceRef]);

    useImperativeHandle(
      ref,
      () => ({
        play,
        pause,
        seekToTime,
        setVolume: setVolumeState,
        setMuted: setMutedState,
        setPlaybackRate,
        getFileSourceNode: () =>
          fileSourceRef.current?.getFileSourceNode() ?? null,
      }),
      [
        pause,
        play,
        seekToTime,
        setMutedState,
        setVolumeState,
        setPlaybackRate,
        fileSourceRef,
      ]
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
  })
);

export default Audio;
