import React, {
  useCallback,
  useEffect,
  useImperativeHandle,
  useMemo,
  useRef,
  useState,
} from 'react';
import { View } from 'react-native';

import type { AudioHandle, AudioProps, AudioTagPlaybackState } from './types';
import { AudioComponentContext } from './AudioTagContext';
import { useStableAudioProps } from './utils';

const Audio = React.forwardRef<AudioHandle, AudioProps>((props, ref) => {
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
    onLoadStart,
    onLoad,
    onError,
    onPositionChanged,
    onSeek,
    onEnded: onEndedCallback,
    onPlay,
    onPause,
    onVolumeChange,
  } = useStableAudioProps(props);

  const audioRef = useRef<HTMLAudioElement>(null);
  const [volumeState, setVolumeState] = useState(volume);
  const [mutedState, setMutedState] = useState(muted);
  const [ready, setReady] = useState(false);
  const [playbackState, setPlaybackState] =
    useState<AudioTagPlaybackState>('idle');
  const [currentTime, setCurrentTime] = useState(0);
  const [duration, setDuration] = useState(0);
  const lastPropMutedRef = useRef(muted);
  const lastPropVolumeRef = useRef(volume);
  const lastElementVolumeRef = useRef(muted ? 0 : volume);
  const isSeekingRef = useRef(false);

  const path = useMemo(() => {
    if (!source) {
      return '';
    }
    if (typeof source === 'string') {
      return source;
    }
    if (typeof source === 'number') {
      throw new Error('Asset source is not supported on web');
    }
    return source.uri ?? '';
  }, [source]);

  useEffect(() => {
    const el = audioRef.current;
    if (!el) {
      return;
    }
    el.loop = loop;
    el.playbackRate = playbackRate;
    // `preservesPitch` is non-standard but supported in modern browsers.
    (el as HTMLAudioElement & { preservesPitch?: boolean }).preservesPitch =
      preservesPitch;
  }, [loop, playbackRate, preservesPitch]);

  useEffect(() => {
    const el = audioRef.current;
    if (!el) {
      return;
    }
    el.volume = mutedState ? 0 : volumeState;
  }, [mutedState, volumeState]);

  useEffect(() => {
    const el = audioRef.current;
    if (!el) {
      return;
    }
    el.muted = mutedState;
  }, [mutedState]);

  useEffect(() => {
    if (lastPropVolumeRef.current !== volume) {
      lastPropVolumeRef.current = volume;
      setVolumeState(volume);
    }
  }, [volume]);

  useEffect(() => {
    if (lastPropMutedRef.current !== muted) {
      lastPropMutedRef.current = muted;
      setMutedState(muted);
    }
  }, [muted]);

  useEffect(() => {
    const el = audioRef.current;
    if (!el) {
      return;
    }

    const handleLoadedMetadata = () => {
      setDuration(el.duration);
      setCurrentTime(el.currentTime);
    };

    const handleTimeUpdate = () => {
      if (!isSeekingRef.current) {
        setCurrentTime(el.currentTime);
        onPositionChanged(el.currentTime);
      }
    };

    const handleSeeked = () => {
      setCurrentTime(el.currentTime);
      onSeek(el.currentTime);
      isSeekingRef.current = false;
    };

    const handleVolumeChange = () => {
      const nextMuted = el.muted;
      const nextVolume = nextMuted ? 0 : el.volume;

      setMutedState(nextMuted);
      setVolumeState(el.volume);

      if (lastElementVolumeRef.current !== nextVolume) {
        lastElementVolumeRef.current = nextVolume;
        onVolumeChange(nextVolume);
      }
    };

    el.addEventListener('loadedmetadata', handleLoadedMetadata);
    el.addEventListener('timeupdate', handleTimeUpdate);
    el.addEventListener('seeked', handleSeeked);
    el.addEventListener('volumechange', handleVolumeChange);
    setDuration(el.duration);
    setCurrentTime(el.currentTime);
    lastElementVolumeRef.current = el.muted ? 0 : el.volume;

    return () => {
      el.removeEventListener('loadedmetadata', handleLoadedMetadata);
      el.removeEventListener('timeupdate', handleTimeUpdate);
      el.removeEventListener('seeked', handleSeeked);
      el.removeEventListener('volumechange', handleVolumeChange);
    };
  }, [onPositionChanged, onSeek, onVolumeChange, ready]);

  const play = useCallback(() => {
    audioRef.current?.play();
    setPlaybackState('playing');
    onPlay();
  }, [onPlay]);

  const pause = useCallback(() => {
    audioRef.current?.pause();
    setPlaybackState('paused');
    onPause();
  }, [onPause]);

  const seekToTime = useCallback(
    (seconds: number) => {
      const el = audioRef.current;
      if (!el) {
        return;
      }
      isSeekingRef.current = true;
      const nextTime =
        duration > 0
          ? Math.max(0, Math.min(seconds, duration))
          : Math.max(0, seconds);
      el.currentTime = nextTime;
      setCurrentTime(nextTime);
    },
    [duration]
  );

  const setVolume = useCallback((next: number) => {
    setVolumeState(next);
  }, []);

  const setMuted = useCallback((next: boolean) => {
    setMutedState(next);
  }, []);

  useImperativeHandle(
    ref,
    () => ({
      play,
      pause,
      seekToTime,
      setVolume,
      setMuted,
    }),
    [pause, play, seekToTime, setMuted, setVolume]
  );

  const ctxValue = useMemo(
    () => ({
      play,
      pause,
      seekToTime,
      setVolume,
      ready,
      volume: volumeState,
      setMuted,
      muted: mutedState,
      playbackState,
      currentTime,
      duration,
      autoPlay,
      controls,
      loop,
      preload,
      playbackRate,
      preservesPitch,
      audioContext: null,
    }),
    [
      play,
      pause,
      seekToTime,
      setVolume,
      ready,
      volumeState,
      setMuted,
      mutedState,
      playbackState,
      currentTime,
      duration,
      autoPlay,
      controls,
      loop,
      preload,
      playbackRate,
      preservesPitch,
    ]
  );

  return (
    <AudioComponentContext.Provider value={ctxValue}>
      <View style={{ alignSelf: 'stretch', width: '100%' }}>
        <audio
          autoPlay={autoPlay}
          controls={controls}
          loop={loop}
          muted={mutedState}
          preload={preload}
          src={path}
          ref={audioRef}
          onLoadStart={() => {
            setReady(false);
            onLoadStart();
          }}
          onLoadedData={() => {
            setReady(true);
            onLoad();
          }}
          onPlay={() => {
            setPlaybackState('playing');
            onPlay();
          }}
          onPause={() => {
            setPlaybackState((state) =>
              state === 'playing' ? 'paused' : state
            );
            onPause();
          }}
          onEnded={() => {
            setPlaybackState('idle');
            setCurrentTime(duration);
            onEndedCallback();
          }}
          onError={() => {
            setReady(false);
            onError(new Error('Failed to load audio element source'));
          }}
        />
        {children}
      </View>
    </AudioComponentContext.Provider>
  );
});

export default Audio;
