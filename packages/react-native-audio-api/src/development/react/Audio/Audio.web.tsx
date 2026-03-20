import React, {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from 'react';
import type { AudioProps, AudioTagPlaybackState } from './types';
import { AudioComponentContext } from './Audio';
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
    context, // null on web, since web do not use AudioContext for audio tag, left for mobile compatibility
  } = useStableAudioProps(props);

  const audioRef = useRef<HTMLAudioElement>(null);
  const [volumeState, setVolumeState] = useState(volume);
  const [mutedState, setMutedState] = useState(muted);
  const [isReady, setIsReady] = useState(false);
  const [playbackState, setPlaybackState] =
    useState<AudioTagPlaybackState>('idle');

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
      volume: volumeState,
      setVolume,
      muted: mutedState,
      setMuted,
      isReady,
      playbackState,
    }),
    [
      play,
      pause,
      setVolume,
      volumeState,
      mutedState,
      setMuted,
      isReady,
      playbackState,
    ]
  );

  return (
    <AudioComponentContext.Provider value={ctxValue}>
      <audio
        autoPlay={autoPlay}
        muted={mutedState}
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
