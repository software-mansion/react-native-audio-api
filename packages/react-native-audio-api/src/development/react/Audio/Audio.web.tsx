import React, {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from 'react';
import type { AudioProps, AudioTagPlaybackState } from './types';
import { AudioComponentContext } from './AudioTagContext';
import { useStableAudioProps } from './utils';

// eslint-disable-next-line @typescript-eslint/no-unused-vars
const Audio: React.FC<AudioProps> = (props) => {
  const { children } = props;
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
  } = useStableAudioProps(props);

  const audioRef = useRef<HTMLAudioElement>(null);
  const [volumeState, setVolumeState] = useState(volume);
  const [mutedState, setMutedState] = useState(muted);
  const [isReady, setIsReady] = useState(false);
  const [playbackState, setPlaybackState] =
    useState<AudioTagPlaybackState>('idle');
  const [currentTime, setCurrentTime] = useState(0);
  const [duration, setDuration] = useState(0);

  const path = useMemo(() => {
    if (typeof source === 'string') {
      return source;
    }
    if (typeof source === 'number') {
      throw new Error('Asset source is not supported on web');
    }
    return source.uri ?? '';
  }, [source]);

  useEffect(() => {
    setVolumeState(volume);
  }, [volume]);

  useEffect(() => {
    setMutedState(muted);
  }, [muted]);

  useEffect(() => {
    const el = audioRef.current;
    if (el) {
      el.volume = volumeState;
    }
  }, [volumeState]);

  useEffect(() => {
    const el = audioRef.current;
    if (!el) return;
    const onLoadedMetadata = () => {
      setDuration(el.duration);
      setCurrentTime(el.currentTime);
    };
    const onTimeUpdate = () => setCurrentTime(el.currentTime);
    el.addEventListener('loadedmetadata', onLoadedMetadata);
    el.addEventListener('timeupdate', onTimeUpdate);
    if (!isNaN(el.duration)) setDuration(el.duration);
    setCurrentTime(el.currentTime);
    return () => {
      el.removeEventListener('loadedmetadata', onLoadedMetadata);
      el.removeEventListener('timeupdate', onTimeUpdate);
    };
  }, [isReady]);

  useEffect(() => {
    const el = audioRef.current;
    if (el) {
      el.muted = mutedState;
    }
  }, [mutedState]);

  const play = useCallback(() => {
    audioRef.current?.play()?.catch(() => {});
  }, []);

  const pause = useCallback(() => {
    audioRef.current?.pause();
  }, []);

  const seekToTime = useCallback(
    (seconds: number) => {
      const el = audioRef.current;
      if (!el) {
        return;
      }
      const d = duration;
      const t =
        d > 0 && Number.isFinite(d)
          ? Math.max(0, Math.min(seconds, d))
          : Math.max(0, seconds);
      if (Number.isFinite(t)) {
        el.currentTime = t;
        setCurrentTime(t);
      }
    },
    [duration]
  );

  const rewind = useCallback(() => {
    seekToTime(0);
  }, [seekToTime]);

  const setVolume = useCallback((next: number) => {
    setVolumeState(next);
    const el = audioRef.current;
    if (el) {
      el.volume = next;
    }
  }, []);

  const setMuted = useCallback((next: boolean) => {
    setMutedState(next);
    const el = audioRef.current;
    if (el) {
      el.muted = next;
    }
  }, []);

  const ctxValue = useMemo(
    () => ({
      play,
      pause,
      rewind,
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
      rewind,
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
      <audio
        autoPlay={autoPlay}
        controls={controls}
        loop={loop}
        muted={mutedState}
        preload={preload}
        src={path}
        ref={audioRef}
        onLoadedData={() => setIsReady(true)}
        onPlay={() => setPlaybackState('playing')}
        onPause={() =>
          setPlaybackState((s) => (s === 'playing' ? 'paused' : s))
        }
        onEnded={() => setPlaybackState('idle')}
      />
      {children}
    </AudioComponentContext.Provider>
  );
};

export default Audio;
