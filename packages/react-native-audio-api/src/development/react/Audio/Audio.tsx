import React, {
  createContext,
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from 'react';

import { IAudioFileSourceNode } from '../../../interfaces';
import type {
  AudioProps,
  AudioTagPlaybackState,
  AudioURISource,
} from './types';
import { useStableAudioProps } from './utils';

export type AudioComponentContextType = {
  play: () => void;
  pause: () => void;
  volume: number;
  setVolume: (volume: number) => void;
  muted: boolean;
  setMuted: (muted: boolean) => void;
  isReady: boolean;
  playbackState: AudioTagPlaybackState;
};

export const AudioComponentContext = createContext<AudioComponentContextType>({
  play: () => {},
  pause: () => {},
  volume: 1,
  setVolume: () => {},
  muted: false,
  setMuted: () => {},
  isReady: false,
  playbackState: 'idle',
});

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
      return source;
    }
    return (source as AudioURISource).uri ?? '';
  }, [source]);

  const nodeRef = useRef<IAudioFileSourceNode | null>(null);
  const [volumeState, setVolumeState] = useState(volume);
  const [mutedState, setMutedState] = useState(muted);
  const [isReady, setIsReady] = useState(false);
  const [playbackState, setPlaybackState] =
    useState<AudioTagPlaybackState>('idle');

  const volumeStateRef = useRef(volumeState);
  volumeStateRef.current = volumeState;

  const mutedStateRef = useRef(mutedState);
  mutedStateRef.current = mutedState;

  const contextRef = useRef(context);
  contextRef.current = context;

  useEffect(() => {
    setVolumeState(volume);
  }, [volume]);

  useEffect(() => {
    setMutedState(muted);
  }, [muted]);

  const play = useCallback(() => {
    const n = nodeRef.current;
    const ctx = contextRef.current;
    if (!n || !ctx) {
      return;
    }
    // @ts-ignore - internal
    n.connect(ctx.destination.node);
    n.start(0);
    setPlaybackState('playing');
  }, []);

  const pause = useCallback(() => {
    if (!nodeRef.current) {
      return;
    }
    nodeRef.current.pause();
    setPlaybackState((s) => (s === 'idle' ? 'idle' : 'paused'));
  }, []);

  const attachNode = useCallback(
    (n: IAudioFileSourceNode) => {
      n.volume = mutedStateRef.current ? 0 : volumeStateRef.current;
      nodeRef.current = n;
      setIsReady(true);
      if (autoPlay) {
        play();
      }
    },
    [autoPlay, play]
  );

  useEffect(() => {
    if (!context || !path) {
      return;
    }

    const run = async () => {
      if (path.startsWith('http')) {
        const response = await fetch(path);
        const arrayBuffer = await response.arrayBuffer();
        attachNode(context.context.createFileSource(arrayBuffer));
      } else {
        attachNode(context.context.createFileSource(path));
      }
    };

    setIsReady(false);
    setPlaybackState('idle');
    run();

    return () => {
      const prev = nodeRef.current;
      if (prev) {
        prev.onEnded = '0';
      }
      prev?.disconnect(undefined);
      nodeRef.current = null;
      setIsReady(false);
      setPlaybackState('idle');
    };
  }, [path, context, attachNode]);

  useEffect(() => {
    const n = nodeRef.current;
    if (n) {
      n.volume = mutedState ? 0 : volumeState;
    }
  }, [volumeState, mutedState]);

  const setVolume = useCallback((next: number) => {
    setVolumeState(next);
    const n = nodeRef.current;
    if (n) {
      n.volume = mutedStateRef.current ? 0 : next;
    }
  }, []);

  const setMuted = useCallback(
    (next: boolean) => {
      setMutedState(next);
      const n = nodeRef.current;
      if (n) {
        if (next) {
          n.volume = 0;
        } else {
          n.volume = volumeState;
        }
      }
    },
    [volumeState]
  );

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

  if (context === null) {
    return null;
  }

  return (
    <AudioComponentContext.Provider value={ctxValue}>
      {children}
    </AudioComponentContext.Provider>
  );
};

export default Audio;
